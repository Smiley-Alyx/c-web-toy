#include "server.h"
#include "http.h"
#include "static.h"
#include "logger.h"
#include "config.h"
#ifdef ENABLE_TLS
#include "tls.h"
#endif

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 16384

// Note: log_init() is called in main(); don't call it again here.

static int make_listen_socket(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) { perror("socket"); return -1; }

    // Reuse addr to avoid TIME_WAIT issues on restart
    int opt = 1;
    (void)setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }
    if (listen(fd, 10) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }
    return fd;
}

void start_server(int port) {
    LOGI("Starting server...");

    int server_fd = make_listen_socket(port);
    if (server_fd < 0) return;

    LOGI("Listening on http://localhost:%d", port);

    // Move big buffer to heap to avoid small thread stack issues (ASan-friendly)
    unsigned char* buffer = (unsigned char*)malloc(BUFFER_SIZE);
    if (!buffer) {
        perror("malloc");
        close(server_fd);
        return;
    }

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr*)&addr, &addr_len);
        if (client_fd < 0) { perror("accept"); continue; }

        size_t total = 0;
        ssize_t n;
        int headers_end = -1;

        while ((n = read(client_fd, buffer + total, BUFFER_SIZE - total - 1)) > 0) {
            total += (size_t)n;
            buffer[total] = '\0';
            char* p = strstr((char*)buffer, "\r\n\r\n");
            if (p) { headers_end = (int)(p - (char*)buffer) + 4; break; }
            if (total >= BUFFER_SIZE - 1) break;
        }
        if (total == 0) { close(client_fd); continue; }

        size_t content_length = 0;
        if (headers_end > 0) {
            const char* line = (const char*)buffer;
            const char* headers_limit = (const char*)buffer + headers_end;
            while (line < headers_limit) {
                const char* line_end = strstr(line, "\r\n");
                if (!line_end) break;
                size_t len = (size_t)(line_end - line);
                if (len == 0) break;

                if (strncasecmp(line, "Content-Length:", 15) == 0) {
                    const char* v = line + 15;
                    while (v < line_end && (*v == ' ' || *v == '\t')) v++;
                    content_length = (size_t)strtoul(v, NULL, 10);
                    break;
                }
                line = line_end + 2;
            }
            size_t already = (total > (size_t)headers_end) ? (total - (size_t)headers_end) : 0;
            while (already < content_length && total < BUFFER_SIZE - 1) {
                n = read(client_fd, buffer + total, BUFFER_SIZE - total - 1);
                if (n <= 0) break;
                total += (size_t)n;
                already = total - (size_t)headers_end;
            }
            buffer[total] = '\0';
        }

        HttpRequest req;
        HttpResponse res;
        http_parse_request(&req, (const char*)buffer, total);
        http_init_response(&res, client_fd);

        // Try static first
        if (static_try_serve(&req, &res)) {
            LOGD("Static served: %s", req.path);
            close(client_fd);
            continue;
        }

        LOGD("Route: %s %s", req.method, req.path);
        http_handle_route(&req, &res);

        close(client_fd);
    }

    // not reached in this toy server
    free(buffer);
    close(server_fd);
}

#ifdef ENABLE_TLS
void start_server_tls(int port, const char* cert_file, const char* key_file) {
    LOGI("Starting TLS server...");

    TlsContext tls = (TlsContext){0};
    if (tls_init(&tls, cert_file, key_file) != 0) {
        LOGE("TLS init failed. Check cert/key files.");
        return;
    }

    int server_fd = make_listen_socket(port);
    if (server_fd < 0) { tls_free(&tls); return; }

    LOGI("Listening (HTTPS) on https://localhost:%d", port);

    unsigned char* buffer = (unsigned char*)malloc(BUFFER_SIZE);
    if (!buffer) {
        perror("malloc");
        close(server_fd);
        tls_free(&tls);
        return;
    }

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr*)&addr, &addr_len);
        if (client_fd < 0) { perror("accept"); continue; }

        SSL* ssl = tls_accept_client(&tls, client_fd);
        if (!ssl) { close(client_fd); continue; }

        // Read request via SSL_read
        size_t total = 0; ssize_t n; int headers_end = -1;
        while ((n = SSL_read(ssl, buffer + total, BUFFER_SIZE - total - 1)) > 0) {
            total += (size_t)n;
            buffer[total] = '\0';
            char* p = strstr((char*)buffer, "\r\n\r\n");
            if (p) { headers_end = (int)(p - (char*)buffer) + 4; break; }
            if (total >= BUFFER_SIZE - 1) break;
        }
        if (total == 0) { SSL_shutdown(ssl); SSL_free(ssl); close(client_fd); continue; }

        size_t content_length = 0;
        if (headers_end > 0) {
            const char* line = (const char*)buffer;
            const char* headers_limit = (const char*)buffer + headers_end;
            while (line < headers_limit) {
                const char* line_end = strstr(line, "\r\n");
                if (!line_end) break;
                size_t len = (size_t)(line_end - line);
                if (len == 0) break;

                if (strncasecmp(line, "Content-Length:", 15) == 0) {
                    const char* v = line + 15;
                    while (v < line_end && (*v == ' ' || *v == '\t')) v++;
                    content_length = (size_t)strtoul(v, NULL, 10);
                    break;
                }
                line = line_end + 2;
            }
            size_t already = (total > (size_t)headers_end) ? (total - (size_t)headers_end) : 0;
            while (already < content_length && total < BUFFER_SIZE - 1) {
                n = SSL_read(ssl, buffer + total, BUFFER_SIZE - total - 1);
                if (n <= 0) break;
                total += (size_t)n;
                already = total - (size_t)headers_end;
            }
            buffer[total] = '\0';
        }

        HttpRequest req;
        HttpResponse res;
        http_parse_request(&req, (const char*)buffer, total);
        http_init_response(&res, client_fd);
        http_response_set_ssl(&res, ssl);

        // Static first
        if (static_try_serve(&req, &res)) {
            SSL_shutdown(ssl); SSL_free(ssl); close(client_fd);
            continue;
        }

        http_handle_route(&req, &res);

        // Graceful TLS close
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client_fd);
    }

    // not reached in this toy server
    free(buffer);
    close(server_fd);
    tls_free(&tls);
}
#endif

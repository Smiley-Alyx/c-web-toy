#include "http.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>

// Routing table
#define MAX_ROUTES 32
typedef struct {
    char method[8];
    char path[256];
    RouteHandler handler;
} Route;

static Route routes[MAX_ROUTES];
static int route_count = 0;

// URL decode
static void url_decode_inplace(char* s) {
    char* w = s; // write
    for (char* r = s; *r; r++) {
        if (*r == '+') {
            *w++ = ' ';
        } else if (*r == '%' && isxdigit((unsigned char)r[1]) && isxdigit((unsigned char)r[2])) {
            char hex[3] = { r[1], r[2], 0 };
            *w++ = (char)strtol(hex, NULL, 16);
            r += 2;
        } else {
            *w++ = *r;
        }
    }
    *w = '\0';
}

// Parse request
void http_parse_request(HttpRequest* req, const char* raw, size_t raw_len) {
    memset(req, 0, sizeof(*req));

    // Request line: METHOD PATH HTTP/1.x
    sscanf(raw, "%7s %255s", req->method, req->path);

    // Query string
    char* q = strchr(req->path, '?');
    if (q) {
        *q = '\0';
        char* qs = q + 1;
        for (char* tok = strtok(qs, "&"); tok && req->query_count < MAX_QUERY_PARAMS; tok = strtok(NULL, "&")) {
            char* eq = strchr(tok, '=');
            if (eq) {
                *eq = '\0';
                strncpy(req->query[req->query_count].key, tok, 63);
                strncpy(req->query[req->query_count].value, eq + 1, 255);
                req->query[req->query_count].key[63] = '\0';
                req->query[req->query_count].value[255] = '\0';
                url_decode_inplace(req->query[req->query_count].key);
                url_decode_inplace(req->query[req->query_count].value);
                req->query_count++;
            }
        }
    }

    // Headers
    const char* header_start = strstr(raw, "\r\n");
    if (!header_start) return;
    header_start += 2;

    while (header_start && *header_start != '\r' && req->header_count < MAX_HEADERS) {
        const char* line_end = strstr(header_start, "\r\n");
        if (!line_end) break;

        char line[512];
        size_t len = (size_t)(line_end - header_start);
        if (len >= sizeof(line)) len = sizeof(line) - 1;
        memcpy(line, header_start, len);
        line[len] = '\0';

        char* colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            char* key = line;
            char* value = colon + 1;
            while (*value == ' ') value++;
            strncpy(req->headers[req->header_count].key, key, 63);
            strncpy(req->headers[req->header_count].value, value, 255);
            req->headers[req->header_count].key[63] = '\0';
            req->headers[req->header_count].value[255] = '\0';

            if (strcasecmp(key, "Content-Type") == 0) {
                strncpy(req->content_type, value, sizeof(req->content_type) - 1);
            }
            req->header_count++;
        }
        header_start = line_end + 2;
    }

    // Body
    const char* sep = strstr(raw, "\r\n\r\n");
    if (!sep) return;

    const char* body = sep + 4;
    size_t available = raw_len - (size_t)(body - raw);
    if (available > MAX_BODY) available = MAX_BODY;

    memcpy(req->body, body, available);
    req->body[available] = '\0';
    req->body_len = available;
}

const char* http_get_header(HttpRequest* req, const char* key) {
    for (int i = 0; i < req->header_count; i++)
        if (strcasecmp(req->headers[i].key, key) == 0) return req->headers[i].value;
    return NULL;
}

const char* http_get_query(HttpRequest* req, const char* key) {
    for (int i = 0; i < req->query_count; i++)
        if (strcmp(req->query[i].key, key) == 0) return req->query[i].value;
    return NULL;
}

// Very simple application/x-www-form-urlencoded parser
const char* http_get_form(HttpRequest* req, const char* key) {
    if (strncasecmp(req->content_type, "application/x-www-form-urlencoded", 33) != 0)
        return NULL;

    static char value[256]; // NOTE: static buffer (not thread-safe)
    char local[MAX_BODY + 1];
    size_t n = req->body_len < MAX_BODY ? req->body_len : MAX_BODY;
    memcpy(local, req->body, n);
    local[n] = '\0';

    for (char* tok = strtok(local, "&"); tok; tok = strtok(NULL, "&")) {
        char* eq = strchr(tok, '=');
        if (eq) {
            *eq = '\0';
            url_decode_inplace(tok);
            url_decode_inplace(eq + 1);
            if (strcmp(tok, key) == 0) {
                strncpy(value, eq + 1, sizeof(value) - 1);
                value[sizeof(value) - 1] = '\0';
                return value;
            }
        }
    }
    return NULL;
}

// Response helpers
void http_init_response(HttpResponse* res, int client_fd) {
    res->client_fd = client_fd;
    res->extra_count = 0;
}

// http_add_header
void http_add_header(HttpResponse* res, const char* key, const char* value) {
    if (res->extra_count >= MAX_EXTRA_HEADERS) return;
    snprintf(res->extra_headers[res->extra_count], EXTRA_HEADER_LEN, "%s: %s", key, value);
    res->extra_count++;
}

// http_set_cookie
// attrs example: "Path=/; HttpOnly; SameSite=Lax; Max-Age=86400"
void http_set_cookie(HttpResponse* res, const char* name, const char* value, const char* attrs) {
    if (res->extra_count >= MAX_EXTRA_HEADERS) return;
    if (!attrs) attrs = "";
    // We store full header line after "Set-Cookie:" to reuse http_add_header format
    snprintf(res->extra_headers[res->extra_count], EXTRA_HEADER_LEN,
             "Set-Cookie: %s=%s%s%s", name, value,
             (attrs && attrs[0]) ? "; " : "", (attrs && attrs[0]) ? attrs : "");
    res->extra_count++;
}

// modify send helpers to print extra headers
static void write_extra_headers(HttpResponse* res, char* header_buf, size_t cap, int* n) {
    for (int i = 0; i < res->extra_count; i++) {
        *n += snprintf(header_buf + *n, cap - (size_t)*n, "%s\r\n", res->extra_headers[i]);
        if (*n >= (int)cap) break;
    }
}

void http_send_text(HttpResponse* res, const char* body) {
    char header[512];
    int n = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Length: %lu\r\n"
        "Connection: close\r\n",
        (unsigned long)strlen(body));
    write_extra_headers(res, header, sizeof(header), &n);
    n += snprintf(header + n, sizeof(header) - (size_t)n, "\r\n");
    write(res->client_fd, header, (size_t)n);
    write(res->client_fd, body, strlen(body));
}

void http_send_html(HttpResponse* res, const char* html) {
    char header[512];
    int n = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %lu\r\n"
        "Connection: close\r\n",
        (unsigned long)strlen(html));
    write_extra_headers(res, header, sizeof(header), &n);
    n += snprintf(header + n, sizeof(header) - (size_t)n, "\r\n");
    write(res->client_fd, header, (size_t)n);
    write(res->client_fd, html, strlen(html));
}

void http_send_bytes(HttpResponse* res, const char* content_type,
                     const unsigned char* data, size_t len) {
    char header[512];
    int n = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %lu\r\n"
        "Connection: close\r\n",
        content_type, (unsigned long)len);
    write_extra_headers(res, header, sizeof(header), &n);
    n += snprintf(header + n, sizeof(header) - (size_t)n, "\r\n");
    write(res->client_fd, header, (size_t)n);
    if (len) write(res->client_fd, data, len);
}

// http_get_cookie
const char* http_get_cookie(HttpRequest* req, const char* name) {
    static char value[256];
    const char* ck = http_get_header(req, "Cookie");
    if (!ck) return NULL;

    // Look for "name=" token
    size_t name_len = strlen(name);
    const char* p = ck;
    while (*p) {
        while (*p == ' ' || *p == ';') p++;
        if (strncmp(p, name, name_len) == 0 && p[name_len] == '=') {
            p += name_len + 1;
            size_t i = 0;
            while (*p && *p != ';' && i + 1 < sizeof(value)) {
                value[i++] = *p++;
            }
            value[i] = '\0';
            return value;
        }
        // skip to next ; or end
        while (*p && *p != ';') p++;
        if (*p == ';') p++;
    }
    return NULL;
}

// Routing
void http_add_route(const char* method, const char* path, RouteHandler handler) {
    if (route_count >= MAX_ROUTES) return;
    strncpy(routes[route_count].method, method, sizeof(routes[route_count].method)-1);
    routes[route_count].method[sizeof(routes[route_count].method)-1] = '\0';
    strncpy(routes[route_count].path, path, sizeof(routes[route_count].path)-1);
    routes[route_count].path[sizeof(routes[route_count].path)-1] = '\0';
    routes[route_count].handler = handler;
    route_count++;
}

void http_handle_route(HttpRequest* req, HttpResponse* res) {
    // Exact match: method + path
    for (int i = 0; i < route_count; i++) {
        if (strcmp(routes[i].path, req->path) == 0 &&
            strcmp(routes[i].method, req->method) == 0) {
            routes[i].handler(req, res);
            return;
        }
    }
    // Path exists with another method → 405
    for (int i = 0; i < route_count; i++) {
        if (strcmp(routes[i].path, req->path) == 0) {
            const char* msg = "405 Method Not Allowed";
            char buf[256];
            int n = snprintf(buf, sizeof(buf),
                "HTTP/1.1 405 Method Not Allowed\r\n"
                "Content-Length: %lu\r\n"
                "Connection: close\r\n\r\n%s",
                (unsigned long)strlen(msg), msg);
            write(res->client_fd, buf, (size_t)n);
            return;
        }
    }
    // Default 404
    http_send_text(res, "404 Not Found");
}

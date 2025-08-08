#include "http.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define MAX_ROUTES 32

static struct {
    char method[8];
    char path[256];
    RouteHandler handler;
} routes[MAX_ROUTES];

static int route_count = 0;

void http_parse_request(HttpRequest* req, const char* raw) {
    req->header_count = 0;
    req->query_count = 0;

    // METHOD and PATH
    sscanf(raw, "%7s %255s", req->method, req->path);

    // query string
    char* question = strchr(req->path, '?');
    if (question) {
        *question = '\0';
        char* query_str = question + 1;
        char* token = strtok(query_str, "&");
        while (token && req->query_count < MAX_QUERY_PARAMS) {
            char* eq = strchr(token, '=');
            if (eq) {
                *eq = '\0';
                strncpy(req->query[req->query_count].key, token, 63);
                strncpy(req->query[req->query_count].value, eq + 1, 255);
                req->query_count++;
            }
            token = strtok(NULL, "&");
        }
    }

    // headers
    const char* header_start = strstr(raw, "\r\n");
    if (!header_start) return;
    header_start += 2;

    while (header_start && *header_start != '\r' && req->header_count < MAX_HEADERS) {
        const char* line_end = strstr(header_start, "\r\n");
        if (!line_end) break;

        char line[512];
        size_t len = line_end - header_start;
        if (len >= sizeof(line)) len = sizeof(line) - 1;
        strncpy(line, header_start, len);
        line[len] = '\0';

        char* colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            char* key = line;
            char* value = colon + 1;
            while (*value == ' ') value++;
            strncpy(req->headers[req->header_count].key, key, 63);
            strncpy(req->headers[req->header_count].value, value, 255);
            req->header_count++;
        }
        header_start = line_end + 2;
    }
}

void http_init_response(HttpResponse* res, int client_fd) {
    res->client_fd = client_fd;
}

void http_send_text(HttpResponse* res, const char* body) {
    char response[1024];
    snprintf(response, sizeof(response),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/plain\r\n"
             "Content-Length: %lu\r\n"
             "Connection: close\r\n\r\n%s",
             (unsigned long)strlen(body), body);
    write(res->client_fd, response, strlen(response));
}

void http_add_route(const char* method, const char* path, RouteHandler handler) {
    if (route_count < MAX_ROUTES) {
        strncpy(routes[route_count].method, method, 7);
        routes[route_count].method[7] = '\0';
        strncpy(routes[route_count].path, path, 255);
        routes[route_count].path[255] = '\0';
        routes[route_count].handler = handler;
        route_count++;
    }
}

void http_handle_route(HttpRequest* req, HttpResponse* res) {
    // First try exact method+path
    for (int i = 0; i < route_count; i++) {
        if (strcmp(routes[i].path, req->path) == 0 &&
            strcmp(routes[i].method, req->method) == 0) {
            routes[i].handler(req, res);
            return;
        }
    }
    // If path exists with another method → 405
    for (int i = 0; i < route_count; i++) {
        if (strcmp(routes[i].path, req->path) == 0) {
            const char* msg = "405 Method Not Allowed";
            char response[256];
            snprintf(response, sizeof(response),
                     "HTTP/1.1 405 Method Not Allowed\r\n"
                     "Content-Length: %lu\r\n"
                     "Connection: close\r\n\r\n%s",
                     (unsigned long)strlen(msg), msg);
            write(res->client_fd, response, strlen(response));
            return;
        }
    }
    // Default 404
    http_send_text(res, "404 Not Found");
}

const char* http_get_header(HttpRequest* req, const char* key) {
    for (int i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->headers[i].key, key) == 0) {
            return req->headers[i].value;
        }
    }
    return NULL;
}

const char* http_get_query(HttpRequest* req, const char* key) {
    for (int i = 0; i < req->query_count; i++) {
        if (strcmp(req->query[i].key, key) == 0) {
            return req->query[i].value;
        }
    }
    return NULL;
}

void http_send_html(HttpResponse* res, const char* html) {
    char response[1024];
    snprintf(response, sizeof(response),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html; charset=utf-8\r\n"
             "Content-Length: %lu\r\n"
             "Connection: close\r\n\r\n%s",
             (unsigned long)strlen(html), html);
    write(res->client_fd, response, strlen(response));
}
void http_send_bytes(HttpResponse* res, const char* content_type, const unsigned char* data, size_t len) {
    char header[256];
    int n = snprintf(header, sizeof(header),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type: %s\r\n"
                     "Content-Length: %lu\r\n"
                     "Connection: close\r\n\r\n",
                     content_type, (unsigned long)len);
    write(res->client_fd, header, (size_t)n);
    if (len > 0) write(res->client_fd, data, len);
}

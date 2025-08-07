#include "http.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_ROUTES 32

static struct {
    char path[256];
    RouteHandler handler;
} routes[MAX_ROUTES];

static int route_count = 0;

void http_parse_request(HttpRequest* req, const char* raw) {
    sscanf(raw, "%7s %255s", req->method, req->path);
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
             strlen(body), body);
    write(res->client_fd, response, strlen(response));
}

void http_add_route(const char* path, RouteHandler handler) {
    if (route_count < MAX_ROUTES) {
        strncpy(routes[route_count].path, path, sizeof(routes[route_count].path) - 1);
        routes[route_count].handler = handler;
        route_count++;
    }
}

void http_handle_route(HttpRequest* req, HttpResponse* res) {
    for (int i = 0; i < route_count; i++) {
        if (strcmp(routes[i].path, req->path) == 0) {
            routes[i].handler(req, res);
            return;
        }
    }
    http_send_text(res, "404 Not Found");
}

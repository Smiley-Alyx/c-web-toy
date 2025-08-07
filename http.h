#ifndef HTTP_H
#define HTTP_H

typedef struct {
    char method[8];
    char path[256];
} HttpRequest;

typedef struct {
    int client_fd;
} HttpResponse;

typedef void (*RouteHandler)(HttpRequest*, HttpResponse*);

void http_parse_request(HttpRequest* req, const char* raw);
void http_init_response(HttpResponse* res, int client_fd);
void http_send_text(HttpResponse* res, const char* body);
void http_add_route(const char* path, RouteHandler handler);
void http_handle_route(HttpRequest* req, HttpResponse* res);

#endif

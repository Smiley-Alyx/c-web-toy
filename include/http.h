#ifndef HTTP_H
#define HTTP_H

#include <stddef.h> 

#define MAX_HEADERS 32
#define MAX_QUERY_PARAMS 32
#define MAX_BODY 8192

typedef struct { char key[64]; char value[256]; } Header;
typedef struct { char key[64]; char value[256]; } QueryParam;

typedef struct {
    char method[8];
    char path[256];
    Header headers[MAX_HEADERS];
    int header_count;
    QueryParam query[MAX_QUERY_PARAMS];
    int query_count;

    char content_type[128];
    char body[MAX_BODY];
    size_t body_len;
} HttpRequest;

typedef struct { int client_fd; } HttpResponse;

typedef void (*RouteHandler)(HttpRequest*, HttpResponse*);

// Parsing & helpers
void http_parse_request(HttpRequest* req, const char* raw, size_t raw_len);
const char* http_get_header(HttpRequest* req, const char* key);
const char* http_get_query(HttpRequest* req, const char* key);
const char* http_get_form(HttpRequest* req, const char* key);

// Response API
void http_init_response(HttpResponse* res, int client_fd);
void http_send_text(HttpResponse* res, const char* body);
void http_send_html(HttpResponse* res, const char* html);
void http_send_bytes(HttpResponse* res, const char* content_type, const unsigned char* data, size_t len);

// Routing
void http_add_route(const char* method, const char* path, RouteHandler handler);
void http_handle_route(HttpRequest* req, HttpResponse* res);

#endif

#include "server.h"
#include "http.h"
#include "template.h"
#include <stdio.h>
#include <stdlib.h>

void route_root(HttpRequest* req, HttpResponse* res) {
    (void)req;
    http_send_text(res, "Welcome to c-web-toy!");
}

void route_hello(HttpRequest* req, HttpResponse* res) {
    http_send_text(res, "Hello, User!");
}

void route_echo(HttpRequest* req, HttpResponse* res) {
    const char* name = http_get_query(req, "name");
    if (!name) name = "stranger";

    char message[256];
    snprintf(message, sizeof(message), "Hello, %s!", name);
    http_send_text(res, message);
}

void route_template(HttpRequest* req, HttpResponse* res) {
    (void)req;
    const char* html = "<html><body><h1>Hello, {{name}}!</h1></body></html>";
    TemplateVar vars[] = { {"name", "stranger"} };

    char* rendered = render_template(html, vars, 1);
    if (rendered) {
        http_send_html(res, rendered);
        free(rendered);
    } else {
        http_send_text(res, "Template rendering failed.");
    }
}

int main() {
    http_add_route("GET", "/", route_root);
    http_add_route("GET", "/hello", route_hello);
    http_add_route("GET", "/echo", route_echo);
    http_add_route("GET", "/template", route_template);

    printf("Запускаем сервер на http://localhost:8080\n");
    start_server(8080);

    return 0;
}

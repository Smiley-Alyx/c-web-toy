#include "server.h"
#include "http.h"
#include <stdio.h>

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

int main() {
    http_add_route("/", route_root);
    http_add_route("/hello", route_hello);
    http_add_route("/echo", route_echo);

    printf("Запускаем сервер на http://localhost:8080\n");
    start_server(8080);

    return 0;
}

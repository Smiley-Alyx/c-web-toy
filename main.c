#include "server.h"
#include "http.h"
#include <stdio.h>

void route_root(HttpRequest* req, HttpResponse* res) {
    http_send_text(res, "Welcome to c-web-toy!");
}

void route_hello(HttpRequest* req, HttpResponse* res) {
    http_send_text(res, "Hello, User!");
}

int main() {
    http_add_route("/", route_root);
    http_add_route("/hello", route_hello);

    printf("Запускаем сервер на http://localhost:8080\n");
    start_server(8080);

    return 0;
}

#include "server.h"
#include "http.h"
#include "template.h"
#include "static.h"
#include <stdio.h>
#include <stdlib.h>

void route_root(HttpRequest* req, HttpResponse* res) {
    (void)req;
    http_send_text(res, "Welcome to c-web-toy!");
}

void route_hello(HttpRequest* req, HttpResponse* res) {
    (void)req;
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

void route_template_file(HttpRequest* req, HttpResponse* res) {
    (void)req;
    TemplateVar vars[] = { {"name", "Alya"} };

    char* rendered = render_template_file("example.html", vars, 1);
    if (rendered) {
        http_send_html(res, rendered);
        free(rendered);
    } else {
        http_send_text(res, "Template file rendering failed.");
    }
}

static void route_form_get(HttpRequest* req, HttpResponse* res) {
    (void)req;
    const char* html =
        "<!doctype html><html><body>"
        "<h1>Form demo</h1>"
        "<form method=\"POST\" action=\"/form\">"
        "Name: <input name=\"name\"><br>"
        "Message: <input name=\"msg\"><br>"
        "<button type=\"submit\">Send</button>"
        "</form>"
        "</body></html>";
    http_send_html(res, html);
}

static void route_form_post(HttpRequest* req, HttpResponse* res) {
    const char* name = http_get_form(req, "name");
    const char* msg  = http_get_form(req, "msg");
    if (!name) name = "(no name)";
    if (!msg)  msg  = "(no message)";

    char out[512];
    snprintf(out, sizeof(out),
        "<!doctype html><html><body>"
        "<h1>Thanks!</h1>"
        "<p>Name: %s</p>"
        "<p>Message: %s</p>"
        "<p><a href=\"/form\">Back</a></p>"
        "</body></html>", name, msg);
    http_send_html(res, out);
}

int main() {
    http_add_route("GET", "/", route_root);
    http_add_route("GET", "/hello", route_hello);
    http_add_route("GET", "/echo", route_echo);
    http_add_route("GET", "/template", route_template);
    http_add_route("GET", "/template-file", route_template_file);
    http_add_route("GET",  "/form", route_form_get);
    http_add_route("POST", "/form", route_form_post);

    printf("Server is running at http://localhost:8080\n");
    static_mount("/static/", "./public");
    start_server(8080);

    return 0;
}

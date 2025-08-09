#include "server.h"
#include "http.h"
#include "template.h"
#include "static.h"
#include "session.h"
#include "logger.h"
#include "config.h"
#include "tls.h"
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

static void route_session(HttpRequest* req, HttpResponse* res) {
    Session* s = session_get_or_create(req, res);
    if (!s) { http_send_text(res, "Session storage full"); return; }
    s->counter++;
    char html[256];
    snprintf(html, sizeof(html),
        "<!doctype html><html><body>"
        "<h1>Session demo</h1>"
        "<p>Your session id: %s</p>"
        "<p>Counter: %d</p>"
        "<p><a href=\"/session\">Refresh</a></p>"
        "</body></html>", s->id, s->counter);
    http_send_html(res, html);
}


int main() {
    log_init();

    char url_prefix[128], static_dir[256];
    config_get_static(url_prefix, sizeof(url_prefix), static_dir, sizeof(static_dir));
    static_mount(url_prefix, static_dir);
    LOGI("Static: %s -> %s", url_prefix, static_dir);

    int port = config_get_port(8080);
    const char* https = getenv("HTTPS");
    const char* cert  = getenv("CERT_FILE");
    const char* key   = getenv("KEY_FILE");
    config_dump();
    
    http_add_route("GET", "/", route_root);
    http_add_route("GET", "/hello", route_hello);
    http_add_route("GET", "/echo", route_echo);
    http_add_route("GET", "/template", route_template);
    http_add_route("GET", "/template-file", route_template_file);
    http_add_route("GET",  "/form", route_form_get);
    http_add_route("POST", "/form", route_form_post);
    http_add_route("GET", "/session", route_session);

    if (https && strcmp(https, "1") == 0 && cert && key) {
        LOGI("Starting HTTPS with cert=%s key=%s", cert, key);
        printf("Server HTTPS is running at http://localhost:8443\n");
        start_server_tls(port, cert, key);
    } else {
        LOGI("Starting HTTP only");
        printf("Server is running at http://localhost:8080\n");
        start_server(port);
    }
    return 0;
}

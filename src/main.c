#include "server.h"
#include "http.h"
#include "template.h"
#include "static.h"
#include "session.h"
#include "logger.h"
#include "config.h"
#ifdef ENABLE_TLS
#include "tls.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef struct { int port; } HttpArgs;

#ifdef ENABLE_TLS
typedef struct { int port; const char* cert; const char* key; } HttpsArgs;
#endif

// Routes

static void route_root(HttpRequest* req, HttpResponse* res) {
    (void)req;
    const char* html =
        "<!doctype html><html><body>"
        "<h1>c-web-toy</h1><p>It works.</p>"
        "<ul>"
        "<li><a href=\"/static/\">Static</a></li>"
        "<li><a href=\"/template\">Template (inline)</a></li>"
        "<li><a href=\"/template-file\">Template (file)</a></li>"
        "<li><a href=\"/form\">Form</a></li>"
        "<li><a href=\"/session\">Session</a></li>"
        "</ul>"
        "</body></html>";
    http_send_html(res, html);
}

static void route_hello(HttpRequest* req, HttpResponse* res) {
    (void)req;
    http_send_text(res, "Hello, User!");
}

static void route_echo(HttpRequest* req, HttpResponse* res) {
    const char* name = http_get_query(req, "name");
    if (!name) name = "stranger";

    char message[256];
    snprintf(message, sizeof(message), "Hello, %s!", name);
    http_send_text(res, message);
}

static void route_template(HttpRequest* req, HttpResponse* res) {
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

static void route_template_file(HttpRequest* req, HttpResponse* res) {
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

// Threads

static void* http_thread(void* arg) {
    // Take ownership of args allocated on heap
    HttpArgs* a = (HttpArgs*)arg;
    int port = a->port;
    free(a);
    start_server(port);
    return NULL;
}

#ifdef ENABLE_TLS
static void* https_thread(void* arg) {
    HttpsArgs* a = (HttpsArgs*)arg;
    int port = a->port;
    const char* cert = a->cert;
    const char* key  = a->key;
    free(a);
    start_server_tls(port, cert, key);
    return NULL;
}
#endif

int main(void) {
    log_init();

    // Static mapping
    char url_prefix[128], static_dir[256];
    config_get_static(url_prefix, sizeof(url_prefix), static_dir, sizeof(static_dir));
    static_mount(url_prefix, static_dir);
    LOGI("Static: %s -> %s", url_prefix, static_dir);

    // Ports
    int http_port  = config_get_port(8080);
    int https_port = 0;

#ifdef ENABLE_TLS
    const char* https = getenv("HTTPS");          // "1" to enable
    const char* cert  = getenv("CERT_FILE");
    const char* key   = getenv("KEY_FILE");
    if (https && strcmp(https, "1") == 0 && cert && key) {
        https_port = config_get_https_port(8443);
    }
#endif

    config_dump();

    // Routes
    http_add_route("GET",  "/",              route_root);
    http_add_route("GET",  "/hello",         route_hello);
    http_add_route("GET",  "/echo",          route_echo);
    http_add_route("GET",  "/template",      route_template);
    http_add_route("GET",  "/template-file", route_template_file);
    http_add_route("GET",  "/form",          route_form_get);
    http_add_route("POST", "/form",          route_form_post);
    http_add_route("GET",  "/session",       route_session);

    // Start HTTP thread (heap-owned args to avoid lifetime issues)
    pthread_t th_http;
    HttpArgs* http_args = (HttpArgs*)malloc(sizeof(*http_args));
    if (!http_args) {
        LOGE("Failed to allocate HTTP args");
        return 1;
    }
    http_args->port = http_port;
    if (pthread_create(&th_http, NULL, http_thread, http_args) != 0) {
        LOGE("Failed to start HTTP thread");
        free(http_args);
        return 1;
    }
    LOGI("HTTP listening on http://localhost:%d", http_port);

#ifdef ENABLE_TLS
    // Start HTTPS thread (optional)
    pthread_t th_https;
    int have_https = (https_port > 0);
    if (have_https) {
        HttpsArgs* https_args = (HttpsArgs*)malloc(sizeof(*https_args));
        if (!https_args) {
            LOGE("Failed to allocate HTTPS args");
            have_https = 0;
        } else {
            https_args->port = https_port;
            https_args->cert = cert;
            https_args->key  = key;
            if (pthread_create(&th_https, NULL, https_thread, https_args) != 0) {
                LOGE("Failed to start HTTPS thread");
                free(https_args);
                have_https = 0;
            } else {
                LOGI("HTTPS listening on https://localhost:%d", https_port);
            }
        }
    }
#endif

    // Join
    pthread_join(th_http, NULL);
#ifdef ENABLE_TLS
    if (https_port > 0) pthread_join(th_https, NULL);
#endif

    return 0;
}

#ifndef STATIC_H
#define STATIC_H

#include "http.h"

// Mount a URL prefix to a local directory
void static_mount(const char* url_prefix, const char* dir);

// Try to serve the current request as a static file.
// Returns 1 if served, 0 otherwise.
int static_try_serve(HttpRequest* req, HttpResponse* res);

#endif

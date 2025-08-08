#include "mime.h"
#include <string.h>

static int ends_with(const char* s, const char* suf) {
    size_t ls = strlen(s), lf = strlen(suf);
    return ls >= lf && strcmp(s + (ls - lf), suf) == 0;
}

const char* mime_from_path(const char* path) {
    if (ends_with(path, ".html") || ends_with(path, ".htm")) return "text/html; charset=utf-8";
    if (ends_with(path, ".css"))  return "text/css; charset=utf-8";
    if (ends_with(path, ".js"))   return "application/javascript; charset=utf-8";
    if (ends_with(path, ".json")) return "application/json; charset=utf-8";
    if (ends_with(path, ".svg"))  return "image/svg+xml";
    if (ends_with(path, ".png"))  return "image/png";
    if (ends_with(path, ".jpg") || ends_with(path, ".jpeg")) return "image/jpeg";
    if (ends_with(path, ".gif"))  return "image/gif";
    if (ends_with(path, ".ico"))  return "image/x-icon";
    if (ends_with(path, ".wasm")) return "application/wasm";
    if (ends_with(path, ".txt"))  return "text/plain; charset=utf-8";
    return "application/octet-stream";
}

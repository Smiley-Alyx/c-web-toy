#include "config.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int config_get_port(int def) {
    const char* p = getenv("PORT");
    if (!p || !*p) return def;
    int v = atoi(p);
    return (v > 0 && v < 65536) ? v : def;
}

int config_get_https_port(int def) {
    const char* p = getenv("HTTPS_PORT");
    if (!p || !*p) return def;
    int v = atoi(p);
    return (v > 0 && v < 65536) ? v : def;
}

void config_get_static(char* url_prefix, int up_len, char* dir, int dir_len) {
    const char* up = getenv("STATIC_URL_PREFIX");
    const char* d = getenv("STATIC_DIR");
    snprintf(url_prefix, up_len, "%s", (up && *up) ? up : "/static/");
    snprintf(dir, dir_len, "%s", (d  && *d ) ? d : "./public");
}

void config_dump(void) {
    const char* p  = getenv("PORT");               if (!p) p = "(default)";
    const char* hp = getenv("HTTPS_PORT");         if (!hp) hp = "(default)";
    const char* up = getenv("STATIC_URL_PREFIX");  if (!up) up = "(default)";
    const char* d  = getenv("STATIC_DIR");         if (!d) d = "(default)";
    const char* ll = getenv("LOG_LEVEL");          if (!ll) ll = "(default)";
    const char* lc = getenv("LOG_COLOR");          if (!lc) lc = "(default)";

    LOGI("Config: PORT=%s, HTTPS_PORT=%s, STATIC_URL_PREFIX=%s, STATIC_DIR=%s, LOG_LEVEL=%s, LOG_COLOR=%s",
         p, hp, up, d, ll, lc);
}

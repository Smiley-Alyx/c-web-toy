#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

static LogLevel g_level = LOG_INFO;
static int g_color = 1;

static const char* level_name(LogLevel l) {
    switch (l) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO:  return "INFO";
        case LOG_WARN:  return "WARN";
        case LOG_ERROR: return "ERROR";
        default:        return "?";
    }
}

static const char* level_color(LogLevel l) {
    switch (l) {
        case LOG_DEBUG: return "\033[36m"; // cyan
        case LOG_INFO:  return "\033[32m"; // green
        case LOG_WARN:  return "\033[33m"; // yellow
        case LOG_ERROR: return "\033[31m"; // red
        default:        return "\033[0m";
    }
}

void log_set_level(LogLevel lvl) { g_level = lvl; }
void log_enable_color(bool enabled) { g_color = enabled ? 1 : 0; }

void log_init(void) {
    const char* lv = getenv("LOG_LEVEL");
    if (lv) {
        if (!strcmp(lv, "DEBUG")) g_level = LOG_DEBUG;
        else if (!strcmp(lv, "INFO")) g_level = LOG_INFO;
        else if (!strcmp(lv, "WARN")) g_level = LOG_WARN;
        else if (!strcmp(lv, "ERROR")) g_level = LOG_ERROR;
    }
    const char* col = getenv("LOG_COLOR");
    if (col) {
        if (!strcmp(col, "0") || !strcasecmp(col, "false")) g_color = 0;
        else g_color = 1;
    }
}

void log_printf(LogLevel lvl, const char* fmt, ...) {
    if (lvl < g_level) return;

    time_t t = time(NULL);
    struct tm* tmv = localtime(&t);
    char ts[20];
    if (tmv) {
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", tmv);
    } else {
        strncpy(ts, "1970-01-01 00:00:00", sizeof(ts));
        ts[sizeof(ts)-1] = '\0';
    }

    const char* cname = level_name(lvl);

    if (g_color) fprintf(stderr, "%s [%s] ", ts, cname), fprintf(stderr, "%s", level_color(lvl));
    else         fprintf(stderr, "%s [%s] ", ts, cname);

    va_list ap; va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    if (g_color) fprintf(stderr, "\033[0m");
    fputc('\n', stderr);
}

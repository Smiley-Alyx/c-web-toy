#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>
#include <stdbool.h>

typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO  = 1,
    LOG_WARN  = 2,
    LOG_ERROR = 3
} LogLevel;

void log_init(void);
void log_set_level(LogLevel lvl);
void log_enable_color(bool enabled);

void log_printf(LogLevel lvl, const char* fmt, ...);

// Convenience macros
#define LOGD(...) log_printf(LOG_DEBUG, __VA_ARGS__)
#define LOGI(...) log_printf(LOG_INFO,  __VA_ARGS__)
#define LOGW(...) log_printf(LOG_WARN,  __VA_ARGS__)
#define LOGE(...) log_printf(LOG_ERROR, __VA_ARGS__)

#endif

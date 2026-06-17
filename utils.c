#include "utils.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#define LOG_FILE "app.log"
#define DEFAULT_LOG_LEVEL 1

static int log_level = DEFAULT_LOG_LEVEL;
static FILE *file = NULL;

void set_log_level(LogLevel level) {
    log_level = level;
}

void reset_log_level() { log_level = DEFAULT_LOG_LEVEL; }

void initialize_logger() {
    file = fopen(LOG_FILE, "a");
    if (file == NULL) {
        perror("Error opening log file");
        return;
    }
}

void cleanup_logger() { fclose(file); }

void _log(LogLevel level, const char *func_name, char *format, ...) {
  if (file == NULL) {
    perror("Logger not initialized");
    return;
    }
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(file, "[%04d-%02d-%02d %02d:%02d:%02d] ", t->tm_year + 1900,
            t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
    fprintf(file, "[%s] ", func_name);
    va_list args;
    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);
    fflush(file);
}

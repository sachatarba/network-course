#include <stdarg.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define LOG_FILE_PATH "../logs/server.log"

static FILE *log_file = NULL;

void init_logger() {
    log_file = fopen(LOG_FILE_PATH, "a");
    if (!log_file) {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }
}

void close_logger() {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}

void log_to_file(const char *format, ...) {
    if (!log_file) {
        fprintf(stderr, "Logger is not initialized\n");
        return;
    }

    time_t raw_time = time(NULL);
    struct tm *time_info = localtime(&raw_time);

    char time_buffer[32];
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", time_info);
    fprintf(log_file, "[%s] ", time_buffer);

    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);

    fprintf(log_file, "\n");
    fflush(log_file);
}
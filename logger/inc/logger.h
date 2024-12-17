#ifndef MAIN_LOGGER_H
#define MAIN_LOGGER_H

#define LOG_FILE_PATH "../logs/server.log"

void init_logger();

void close_logger();

void log_to_file(const char *format, ...);

#endif //MAIN_LOGGER_H

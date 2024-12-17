#ifndef MAIN_ERRORS_H
#define MAIN_ERRORS_H

typedef enum {
    POOL_OK,
    CANT_CLOSE_FILE,
    CANT_CLOSE_SOCKET,
    POOL_OVERFLOW,
    NO_EVENTS
} request_pool_errors_t;

#endif //MAIN_ERRORS_H

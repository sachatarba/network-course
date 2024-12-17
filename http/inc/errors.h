#ifndef HTTP_ERRORS_H
#define HTTP_ERRORS_H

typedef enum {
    OK,
    ERR_SENDING,
    ERR_SEEK_FILE,
    ERR_READ_FILE,
} http_errors_t;

#endif //HTTP_ERRORS_H

#ifndef MAIN_HTTP_H
#define MAIN_HTTP_H

#include "errors.h"

#define MAX_HEADERS_COUNT 64
#define MAX_HEADER_SIZE 256

typedef enum {
    HTTP_OK = 200,
    HTTP_CREATED = 201,
    HTTP_ACCEPTED = 202,
    HTTP_NO_CONTENT = 204,
    HTTP_BAD_REQUEST = 400,
    HTTP_UNAUTHORIZED = 401,
    HTTP_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_METHOD_NOT_ALLOWED = 405,
    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_NOT_IMPLEMENTED = 501,
    HTTP_BAD_GATEWAY = 502,
    HTTP_SERVICE_UNAVAILABLE = 503
} http_status_t;

http_errors_t send_response(int fd, http_status_t code, char headers[][MAX_HEADER_SIZE], size_t headers_count, const char *body);

http_errors_t send_response_headers(int fd, http_status_t code, char headers[][MAX_HEADER_SIZE], size_t headers_count, size_t body_size);

http_errors_t send_bytes_from_file(int socket_fd, int fd, off_t offset, size_t nbytes);

const char* get_http_status_text(http_status_t status);

#endif //MAIN_HTTP_H

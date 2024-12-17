#ifndef MAIN_REQUEST_H
#define MAIN_REQUEST_H

#include <stdio.h>

typedef enum {
    FREE,
    READING,
    HAS_READ,
    WRITING,
    CONN_CLOSED,
} request_status_t;

typedef struct {
    int socket_fd;

    int file_fd;
    size_t file_size;
    off_t offset;

    char filepath[256];

    char method[256];

    request_status_t status;
} request_t;

request_t create_request(int socket_fd);

int process_request(request_t  *request);

int was_conn_died(request_t *request);

// request is in static memory, only destroys objects of request_t struct
void destroy_request(request_t *request);

#endif //MAIN_REQUEST_H

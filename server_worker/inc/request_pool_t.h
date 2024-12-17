#ifndef MAIN_REQUEST_POOL_H
#define MAIN_REQUEST_POOL_H

#include <sys/select.h>

#include "request_t.h"
#include "errors.h"

typedef struct {
    request_t requests[FD_SETSIZE];
    size_t request_count;

    int max_fd;

    fd_set rfds;
    fd_set wfds;

    int listener_socket;
} request_pool_t;

request_pool_t *create_request_pool(int listener_socket_fd);

void track_requests(request_pool_t *pool);

request_pool_errors_t remove_request(request_pool_t *pool, size_t request_index);

request_pool_errors_t add_request(request_pool_t *pool, request_t *request);

int wait_events(request_pool_t *pool, struct timeval *timeout);

int can_read(request_pool_t *pool, size_t request_index);

int can_write(request_pool_t *pool, size_t request_index);

#endif //MAIN_REQUEST_POOL_H

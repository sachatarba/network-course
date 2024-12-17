#include <unistd.h>
#include <malloc.h>
#include <stdlib.h>

#include "../inc/request_pool_t.h"

request_pool_t *create_request_pool(int listener_socket_fd) {
    request_pool_t *pool = malloc(sizeof(request_pool_t));
    pool->listener_socket = listener_socket_fd;

    request_t request = create_request(pool->listener_socket);
    add_request(pool, &request);

    return pool;
}

request_pool_errors_t free_request_pool(request_pool_t *pool) {
    request_pool_errors_t error;
    for (size_t i = pool->request_count; i != (size_t)-1; --i) {
        error = remove_request(pool, i);
        if (error != POOL_OK) {
            break;
        }
    }

    if (error != POOL_OK) {
        return error;
    }

    free(pool);

    return POOL_OK;
}

void track_requests(request_pool_t *pool) {
    FD_ZERO(&pool->rfds);
    FD_ZERO(&pool->wfds);

    request_t request;
    for (size_t i = 0; i < pool->request_count; ++i) {
        request = pool->requests[i];
        if (request.status == CONN_CLOSED) {
            remove_request(pool, i);
        }
        FD_SET(request.socket_fd, &pool->rfds);
        if (request.status == WRITING) {
            FD_SET(request.socket_fd, &pool->wfds);
        }

        pool->max_fd = pool->max_fd > request.socket_fd ? pool->max_fd : request.socket_fd;
    }
}

request_pool_errors_t remove_request(request_pool_t *pool, size_t request_index) {
    printf("\nhere\n");
    int rc = close(pool->requests[request_index].file_fd);

    rc = close(pool->requests[request_index].socket_fd);
    if (rc == -1) {
        return CANT_CLOSE_SOCKET;
    }

    destroy_request(&pool->requests[request_index]);
    for (size_t i = request_index; i < pool->request_count - 1; i++) {
        pool->requests[i] = pool->requests[i + 1];
    }
    pool->request_count--;

    return POOL_OK;
}

request_pool_errors_t add_request(request_pool_t *pool, request_t *request) {
    if (pool->request_count > FD_SETSIZE) {
        return POOL_OVERFLOW;
    }

    pool->requests[pool->request_count++] = *request;
    pool->max_fd = pool->max_fd > request->socket_fd ? pool->max_fd : request->socket_fd;

    return POOL_OK;
}

int wait_events(request_pool_t *pool, struct timeval *timeout) {
    return select(pool->max_fd + 1, &pool->rfds, &pool->wfds, NULL, timeout);
}

int can_read(request_pool_t *pool, size_t request_index) {
    return FD_ISSET(pool->requests[request_index].socket_fd, &pool->rfds);
}

int can_write(request_pool_t *pool, size_t request_index) {
    return FD_ISSET(pool->requests[request_index].socket_fd, &pool->wfds);
}

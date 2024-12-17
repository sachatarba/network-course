#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>


#include "../inc/server_worker_t.h"
#include "../inc/request_pool_t.h"
#include "../../logger/inc/logger.h"


#define ROOT_DIR "../www"
#define MAX_FILE_SIZE 134217728 // 128MB

#define BUFF_SIZE BUFSIZ

static int start_worker(server_worker_t *worker);

static int accept_request(request_pool_t *pool);

static int handle_client(request_pool_t *pool, size_t request_index);

server_worker_t *create_server_worker(int server_socket_fd) {
    server_worker_t *worker = malloc(sizeof(server_worker_t));
    if (worker == NULL) {
        log_to_file("master[%d]: can't malloc worker", getpid());
        return NULL;
    }

    pid_t pid = fork();
    if (pid == -1) {
        free(worker);
        log_to_file("master[%d]: can't fork worker", getpid());
        return NULL;
    } else if (pid == 0) {
        log_to_file("worker[%d]: successfully forked", getpid());
        worker->server_socket_fd = server_socket_fd;
        start_worker(worker);
        return NULL;
    } else {
        worker->pid = pid;
        log_to_file("master[%d]: worker created with PID %d", getpid(), pid);
        return worker;
    }
}

static int start_worker(server_worker_t *worker) {
    log_to_file("worker[%d]: started worker loop", getpid());

    struct timeval tv;
    tv.tv_sec = 20;
    tv.tv_usec = 0;

    request_pool_t *pool = create_request_pool(worker->server_socket_fd);
    track_requests(pool);

    size_t cur_request_count = 0;
    for (;;) {
        int retval = wait_events(pool, &tv);

        if (retval > 0) {
            log_to_file("worker[%d]: %d fds selected", getpid(), retval);

            cur_request_count = pool->request_count;
            for (size_t i = 0; i < cur_request_count; ++i) {
                if (can_read(pool, i) || can_write(pool, i)) {
                    if (pool->requests[i].socket_fd == pool->listener_socket) {
                        log_to_file("worker[%d]: accepting new request", getpid());
                        accept_request(pool);
                    } else {
                        log_to_file("worker[%d]: handling client request %zu", getpid(), i);
                        handle_client(pool, i);
                    }
                }
            }
        } else if (retval == 0) {
            log_to_file("worker[%d]: no fds to select", getpid());
        } else {
            log_to_file("worker[%d]: error on select", getpid());
        }

        track_requests(pool);
        tv.tv_sec = 20;
        tv.tv_usec = 0;
    }
}


static int handle_client(request_pool_t *pool, size_t request_index) {
    return process_request(&pool->requests[request_index]);
}

static int accept_request(request_pool_t *pool) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int client_socket_fd = accept(pool->listener_socket, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_socket_fd < 0) {
        log_to_file("worker[%d]: failed to accept connection", getpid());
        perror("accept");
        return -1;
    }

    log_to_file("worker[%d]: accepted connection from %s:%d",
                getpid(), inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    request_t request = create_request(client_socket_fd);
    add_request(pool, &request);

    return 0;
}

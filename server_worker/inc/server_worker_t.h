#ifndef COURSE_SERVER_WORKER_T_H
#define COURSE_SERVER_WORKER_T_H

#include <sys/socket.h>

typedef enum {
    FREE,
    BUSY,
} server_worker_state_t;


typedef struct {
    server_worker_state_t worker_state; // State of server worker

    // int worker_socket_fd; // Worker's socket of socketpair
    // int master_socket_fd; //

    int server_socket_fd;
    int log_fd;

    pid_t pid;
} server_worker_t;

typedef struct {
    int client_socket_fd
};

server_worker_t* create_server_worker(int server_socket_fd);

int free_server_worker(server_worker_t* worker);


#endif //COURSE_SERVER_WORKER_T_H

#ifndef COURSE_SERVER_WORKER_T_H
#define COURSE_SERVER_WORKER_T_H

#include <sys/socket.h>

typedef struct {
    int server_socket_fd;
    int log_fd;

    pid_t pid;
} server_worker_t;


server_worker_t* create_server_worker(int server_socket_fd);


#endif //COURSE_SERVER_WORKER_T_H

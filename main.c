#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <fcntl.h>

#define PORT 8080

#include "server_worker/inc/server_worker_t.h"

#define MAX_CONNECTIONS 20000
#define PROCESSES 4

void set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}


int main() {
    int server_fd;
    struct sockaddr_in server_addr;
    socklen_t server_addr_len = sizeof(server_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    set_nonblocking(server_fd);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, server_addr_len) == -1) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) == -1) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", PORT);

    server_worker_t *workers[PROCESSES];
    for (int i = 0; i < PROCESSES; ++i) {
        workers[i] = create_server_worker(server_fd);
    }

    for (int i = 0; i < PROCESSES; ++i) {
        int status;
        waitpid(workers[i]->pid, &status, 0);

        if (WIFSIGNALED(status)) {
            printf("\nmaster[%d]: waited for worker exit error\n", getpid());
        } else if (WEXITSTATUS(status)) {
            printf("\nmaster[%d]: waited for worker exit normally\n", getpid());
        }
    }

    for (int i = 0; i < PROCESSES; ++i) {
        free(workers[i]);
    }

    close(server_fd);
}


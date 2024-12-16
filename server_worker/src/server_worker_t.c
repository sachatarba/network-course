#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include "../inc/server_worker_t.h"

#define MAX_FILE_CHUNK 4096
#define ROOT_DIR "../www"
#define MAX_FILE_SIZE 134217728 // 128MB

#define BUFF_SIZE 4096

static int start_worker(server_worker_t *worker);

static int accept_client(int *fds, int *openned_socket_count, int *max_fd);

static int handle_client(int *fds, int *openned_socket_count, char *buff, int i);

static void perrorf(char *message);

static void remove_fd(int *fds, int *count, int index);

static const char *get_content_type(const char *filename);


server_worker_t *create_server_worker(int server_socket_fd) {
    server_worker_t *worker = malloc(sizeof(server_worker_t));
    if (worker == NULL) {
        char buff[128];
        sprintf(buff, "\nmaster[%d]: can't malloc worker\n", getpid());
        perror(buff);
        // log

        return NULL;
    }

    pid_t pid = fork();
    if (pid == -1) {
        free(worker);
        char buff[128];
        sprintf(buff, "\nmaster[%d]: can't fork worker\n", getpid());
        perror(buff);
        // log

        return NULL;
    }
    else if (pid == 0) {
        printf("\nworker[%d]: succesfully forked\n", getpid());
        worker->server_socket_fd = server_socket_fd;
        start_worker(worker);

        return NULL;
    }
    else {
        worker->pid = pid;
        printf("\nmaster[%d]: worker created\n", getpid());

        return worker;
    }
}

int free_server_worker(server_worker_t *worker);


static int start_worker(server_worker_t *worker) {
    printf("\nworker[%d]: started worker loop\n", getpid());

    int retval;

    int fds[FD_SETSIZE];
    int openned_socket_count = 1;

    char buff[BUFF_SIZE] = "\0";

    struct timeval tv;
    tv.tv_sec = 20;
    tv.tv_usec = 0;

    fds[0] = worker->server_socket_fd;
    int max_fd = fds[0];

    fd_set rfds;
    FD_ZERO(&rfds);

    FD_SET(fds[0], &rfds);

    for (;;) {
        retval = select(max_fd + 1, &rfds, NULL, NULL, &tv);

        if (retval) {
            printf("\nworker[%d]: fds selected\n", getpid());

            for (int i = 0; i < openned_socket_count; ++i) {
                if (FD_ISSET(fds[i], &rfds)) {
                    if (i == 0) {
                        accept_client(fds, &openned_socket_count, &max_fd);
                    } else {
                        handle_client(fds, &openned_socket_count, buff, i);
                    }
                }
            }
        }
        else {
            printf("\nworker[%d]: no fds to select\n", getpid());
        }

        FD_ZERO(&rfds);

        tv.tv_sec = 20;
        tv.tv_usec = 0;

        for (int i = 0; i < openned_socket_count; ++i) {
            FD_SET(fds[i], &rfds);
        }
    }
}

static void send_response(int client_fd, int status_code, const char *status_text, const char *content_type, const char *body, size_t body_length) {
    char header[BUFF_SIZE];
    snprintf(header, sizeof(header),
             "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n\r\n",
             status_code, status_text, content_type, body_length);
    if (send(client_fd, header, strlen(header), 0) == -1) {
        return;
    }
    if (body && body_length > 0) {
        send(client_fd, body, body_length, 0);
    }
}

static int is_path_safe(const char *path) {
    return strstr(path, "..") == NULL;
}


static void send_file_response(int client_fd, const char *file_path, const char *method) {
    struct stat file_stat;
    if (stat(file_path, &file_stat) < 0 || S_ISDIR(file_stat.st_mode)) {
        send_response(client_fd, 404, "Not Found", "text/plain", "File Not Found", strlen("File Not Found"));
        return;
    }

    if (file_stat.st_size > MAX_FILE_SIZE || access(file_path, R_OK) != 0) {
        send_response(client_fd, 403, "Forbidden", "text/plain", "Access Forbidden", strlen("Access Forbidden"));
        return;
    }

    int file_fd = open(file_path, O_RDONLY);
    if (file_fd < 0) {
        send_response(client_fd, 500, "Internal Server Error", "text/plain", "Internal Server Error", strlen("Internal Server Error"));
        return;
    }

    const char *content_type = get_content_type(file_path);

    printf("worker[%d]: start sending: %s\n", getpid(), file_path);

    send_response(client_fd, 200, "OK", content_type, NULL, file_stat.st_size);

    if (strcmp(method, "GET") == 0) {
        char buffer[MAX_FILE_CHUNK];
        ssize_t bytes_read;
        while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
            if (send(client_fd, buffer, bytes_read, 0) == -1) {
                break;
            }
        }
    }
    if (close(file_fd) == -1) {
        perrorf("worker[%d]: can't close file");
    }
}

static int handle_client(int *fds, int *openned_socket_count, char *buff, int i) {
    int client_fd = fds[i];
    ssize_t bytes_read = recv(client_fd, buff, BUFF_SIZE - 1, 0);

    if (bytes_read <= 0) {
        remove_fd(fds, openned_socket_count, i);
        return -1;
    }

    buff[bytes_read] = '\0';
    char method[16], path[256], version[16];
    sscanf(buff, "%15s %255s %15s", method, path, version);

    printf("worker[%d]: received request: %s %s %s\n", getpid(), method, path, version);

    if (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0) {
        send_response(client_fd, 405, "Method Not Allowed", "text/plain", "Method Not Allowed", strlen("Method Not Allowed"));
        // remove_fd(fds, openned_socket_count, i);
        return -1;
    }

    if (!is_path_safe(path)) {
        send_response(client_fd, 404, "Not Found", "text/plain", "Not Found", strlen("Not Found"));
        return -1;
    }

    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s%s", ROOT_DIR, path);
    if (strcmp(path, "/") == 0) {
        snprintf(file_path, sizeof(file_path), "%s/index.html", ROOT_DIR);
    }

    printf("worker[%d]: attempt to send file: %s\n", getpid(), file_path);

    send_file_response(client_fd, file_path, method);

    printf("worker[%d]: file sended: %s\n", getpid(), file_path);

    // remove_fd(fds, openned_socket_count, i);
    return 0;
}

static int lock_file(int fd) {
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    if (fcntl(fd, F_SETLK, &fl) == -1) {
        if (errno == EACCES || errno == EAGAIN) {
            printf("\nworker[%d]: file is already locked by another process\n", getpid());
            return -1;
        }

        perrorf("worker[%d]: failed to set lock");
        return -1;
    }

    printf("\nworker[%d]: locked file\n", getpid());

    return 0;
}

static int unlock_file(int fd) {
    struct flock fl;
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    if (fcntl(fd, F_SETLK, &fl) == -1) {
        perrorf("worker[%d]: failed to release lock");
        return -1;
    }
    printf("\nworker[%d]: lock released\n", getpid());

    return 0;
}

static int accept_client(int *fds, int *openned_socket_count, int *max_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

//    if (lock_file(fds[0]) == -1) {
//        return -1;
//    }

    int client_socket_fd = accept(fds[0], (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_socket_fd < 0) {
        perrorf("worker[%d]: can't accept");
        return -1;
    }

    fds[(*openned_socket_count)++] = client_socket_fd;
//    FD_SET(client_socket_fd, rfds);
    *max_fd = *max_fd > client_socket_fd ? *max_fd : client_socket_fd;
    printf("\nworker[%d]: accepted\n", getpid());

//    if (!unlock_file(fds[0])) {
//        return -1;
//    }

    return 0;
}

static void perrorf(char *message) {
    char msg[128];
    sprintf(msg, message, getpid());
    perror(msg);
}

static void remove_fd(int *fds, int *count, int index) {
    int n = close(fds[index]);
    if (n == -1) {
        perrorf("worker[%d]: can't close");
    }
    for (int i = index; i < *count - 1; i++) {
        fds[i] = fds[i + 1];
    }
    (*count)--;
}

static const char *get_content_type(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext || ext == filename) {
        return "application/octet-stream";
    }

    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) {
        return "text/html";
    } else if (strcmp(ext, ".css") == 0) {
        return "text/css";
    } else if (strcmp(ext, ".js") == 0) {
        return "application/javascript";
    } else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) {
        return "image/jpeg";
    } else if (strcmp(ext, ".png") == 0) {
        return "image/png";
    } else if (strcmp(ext, ".gif") == 0) {
        return "image/gif";
    } else if (strcmp(ext, ".svg") == 0) {
        return "image/svg+xml";
    } else if (strcmp(ext, ".ico") == 0) {
        return "image/x-icon";
    } else if (strcmp(ext, ".json") == 0) {
        return "application/json";
    } else if (strcmp(ext, ".xml") == 0) {
        return "application/xml";
    } else if (strcmp(ext, ".pdf") == 0) {
        return "application/pdf";
    } else if (strcmp(ext, ".zip") == 0) {
        return "application/zip";
    } else if (strcmp(ext, ".mp4") == 0) {
        return "video/mp4";
    } else if (strcmp(ext, ".mp3") == 0) {
        return "audio/mpeg";
    } else if (strcmp(ext, ".wav") == 0) {
        return "audio/wav";
    } else if (strcmp(ext, ".txt") == 0) {
        return "text/plain";
    }

    return "application/octet-stream";
}

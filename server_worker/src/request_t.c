#include <string.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include "../inc/request_t.h"
#include "http.h"
#include "files.h"
#include "file_check_status.h"
#include "../../logger/inc/logger.h"

#define ROOT_DIR "../www"

static int write_request(request_t *request);

static int read_request(request_t *request);

request_t create_request(int socket_fd) {
    request_t request = {
            .socket_fd = socket_fd,
            .file_fd = -1,
            .status = FREE,
            .offset = 0,
    };

    return request;
}


int process_request(request_t  *request) {
    if (request->status == FREE) {
        return read_request(request);
    }

    if (request->status == WRITING) {
        return write_request(request);
    }

    return -1;
}

static int read_request(request_t *request) {
    if (request->status != FREE) {
        return -1;
    }

    char buff[BUFSIZ];
    ssize_t bytes_read = recv(request->socket_fd, buff, BUFSIZ - 1, 0);
    if (bytes_read <= 0) {
        request->status = CONN_CLOSED;
        log_to_file("worker[%d]: failed to read from socket: %d", getpid(), request->socket_fd);
        return 0;
    }

    buff[bytes_read] = '\0';

    log_to_file("worker[%d]: get request from socket %d: %s", getpid(), request->socket_fd, buff);

    char method[16], path[256], version[16];
    sscanf(buff, "%15s %255s %15s", method, path, version);


    char headers[MAX_HEADERS_COUNT][MAX_HEADER_SIZE];
    int body_size = 0;
    int headers_count = 0;


    if (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0) {
        send_response_headers(request->socket_fd, HTTP_METHOD_NOT_ALLOWED, headers, headers_count, body_size);
        request->status = FREE;
        return 0;
    }


    if (!is_path_safe(path)) {
        send_response_headers(request->socket_fd, HTTP_FORBIDDEN, headers, headers_count, body_size);
        request->status = FREE;
        return 0;
    }

    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s%s", ROOT_DIR, path);
    if (strcmp(path, "/") == 0) {
        snprintf(file_path, sizeof(file_path), "%s/index.html", ROOT_DIR);
    }

    file_check_status_t status = check_filepath(file_path);
    if (status == NOT_FOUND) {
        send_response_headers(request->socket_fd, HTTP_NOT_FOUND, headers, headers_count, body_size);
        request->status = FREE;
        return 0;
    }

    if (status == FORBIDDEN || status == TOO_LARGE_FILE) {
        send_response_headers(request->socket_fd, HTTP_FORBIDDEN, headers, headers_count, body_size);
        request->status = FREE;
        return 0;
    }


    strcpy(request->method, method);
    strcpy(request->filepath, file_path);
    request->status = HAS_READ;
    write_request(request);

    return 0;
}

static int write_request(request_t *request) {
    char headers[MAX_HEADERS_COUNT][MAX_HEADER_SIZE];
    int body_size = 0;
    int headers_count = 0;

    if (request->status == HAS_READ) {
        int file_fd = open(request->filepath, O_RDONLY);
        if (file_fd < 0) {
            send_response_headers(request->socket_fd, HTTP_INTERNAL_SERVER_ERROR, headers, headers_count, body_size);
            request->status = FREE;
            request->method[0] = '\0';
            request->filepath[0] = '\0';
            return -1;
        }

        request->file_fd = file_fd;
        request->file_size = get_file_size(request->filepath);

        const char *content_type = get_content_type(request->filepath);
        sprintf(headers[0], "Content-Type: %s", content_type);

        send_response_headers(request->socket_fd, 200, headers, 1, request->file_size);
        if (strcmp(request->method, "HEAD") == 0) {
            request->status = FREE;
            request->method[0] = '\0';
            request->filepath[0] = '\0';
            request->file_size = 0;
            close(request->file_fd);
            request->file_fd = -1;

            return 0;
        }
    }

    if (request->status != HAS_READ && request->status != WRITING) {
        return -1;
    }

    send_bytes_from_file(request->socket_fd, request->file_fd, request->offset, BUFSIZ);
    request->offset += BUFSIZ;
    if (request->offset >= request->file_size) {
        request->status = FREE;
        request->offset = 0;
        request->method[0] = '\0';
        request->filepath[0] = '\0';
        request->file_size = 0;
        close(request->file_fd);
        request->file_fd = -1;

        return 0;
    }

    request->status = WRITING;

    return 0;
}


int was_conn_died(request_t *request) {
    return request->status == CONN_CLOSED;
}

void destroy_request(request_t *request) {
    if (request->status == WRITING) {
        close(request->file_fd);
        request->file_fd = -1;
        return;
    }

    close(request->socket_fd);
}

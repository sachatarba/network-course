#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../inc/http.h"

#define BUFF_SIZE BUFSIZ

#define HTTP_HEADERS_SIZE 1024

http_errors_t send_response(int fd, http_status_t code, char headers[][MAX_HEADER_SIZE], size_t headers_count, const char *body) {
    if (send_response_headers(fd, code, headers, headers_count, strlen(body)) != OK) {
        return ERR_SENDING;
    }

    if (send(fd, body, strlen(body), 0) < 0) {
        return ERR_SENDING;
    }

    return OK;
}

http_errors_t send_bytes_from_file(int socket_fd, int fd, off_t offset, size_t nbytes) {
    if (lseek(fd, offset, SEEK_SET) == -1) {
        return ERR_SEEK_FILE;
    }
    char buffer[BUFF_SIZE];

    ssize_t bytes_read = read(fd, buffer, nbytes);
    if (bytes_read < 0) {
        return ERR_READ_FILE;
    }

    if (send(socket_fd, buffer, bytes_read, 0) < 0) {
        return ERR_SENDING;
    }

    return OK;
}

http_errors_t send_response_headers(int fd, http_status_t code, char headers[][MAX_HEADER_SIZE], size_t headers_count, size_t body_size) {
    char header[HTTP_HEADERS_SIZE];
    const char *status_text = get_http_status_text(code);
    size_t offset = 0;

    int written = snprintf(header, sizeof(header),
                           "HTTP/1.1 %d %s\r\nContent-Length: %zu\r\n", code, status_text, body_size);
    if (written < 0 || (size_t)written >= sizeof(header)) {
        return ERR_SENDING;
    }
    offset += written;

    for (size_t i = 0; i < headers_count; i++) {
        if (strlen(headers[i]) == 0) {
            continue;
        }
        written = snprintf(header + offset, sizeof(header) - offset,
                           "%s\r\n", headers[i]);
        if (written < 0 || (size_t)written >= (sizeof(header) - offset)) {
            return ERR_SENDING;
        }
        offset += written;
    }

    if (offset + 2 >= sizeof(header)) {
        return ERR_SENDING;
    }
    header[offset++] = '\r';
    header[offset++] = '\n';

    if (send(fd, header, offset, 0) == -1) {
        perror("send");
        return ERR_SENDING;
    }

    return OK;
}

const char *get_http_status_text(http_status_t status) {
    switch (status) {
        case HTTP_OK:
            return "OK";
        case HTTP_CREATED:
            return "Created";
        case HTTP_ACCEPTED:
            return "Accepted";
        case HTTP_NO_CONTENT:
            return "No Content";
        case HTTP_BAD_REQUEST:
            return "Bad Request";
        case HTTP_UNAUTHORIZED:
            return "Unauthorized";
        case HTTP_FORBIDDEN:
            return "Forbidden";
        case HTTP_NOT_FOUND:
            return "Not Found";
        case HTTP_METHOD_NOT_ALLOWED:
            return "Method Not Allowed";
        case HTTP_INTERNAL_SERVER_ERROR:
            return "Internal Server Error";
        case HTTP_NOT_IMPLEMENTED:
            return "Not Implemented";
        case HTTP_BAD_GATEWAY:
            return "Bad Gateway";
        case HTTP_SERVICE_UNAVAILABLE:
            return "Service Unavailable";
        default:
            return "Unknown Status";
    }
}

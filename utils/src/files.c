#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../inc/files.h"
#include "../inc/file_check_status.h"


int is_path_safe(const char *path) {
    return strstr(path, "..") == NULL;
}

int check_filepath(const char *path) {
    struct stat file_stat;
    if (stat(path, &file_stat) < 0 || S_ISDIR(file_stat.st_mode)) {
        return NOT_FOUND;
    }

    if (access(path, R_OK) != 0) {
        return FORBIDDEN;
    }

    if (file_stat.st_size > MAX_FILE_SIZE) {
        return  TOO_LARGE_FILE;
    }

    return FILE_STATUS_OK;
}

ssize_t get_file_size(const char *path) {
    struct stat file_stat;
    if (stat(path, &file_stat) < 0 || S_ISDIR(file_stat.st_mode)) {
        return -1;
    }

    return file_stat.st_size;
}

const char *get_content_type(const char *filename) {
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

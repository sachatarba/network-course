#ifndef MAIN_CHECK_STATUS_H
#define MAIN_CHECK_STATUS_H

typedef enum {
    FILE_STATUS_OK,
    NOT_FOUND,
    FORBIDDEN,
    TOO_LARGE_FILE
} file_check_status_t;

#endif //MAIN_CHECK_STATUS_H

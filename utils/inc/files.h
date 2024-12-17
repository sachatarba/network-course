#ifndef MAIN_FILES_H
#define MAIN_FILES_H

#define MAX_FILE_SIZE 134217728 // 128MB

int is_path_safe(const char *path);

int check_filepath(const char *path);

ssize_t get_file_size(const char *path);

const char *get_content_type(const char *filename);

#endif //MAIN_FILES_H

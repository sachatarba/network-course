cmake_minimum_required(VERSION 3.10)
project(main C)

set(CMAKE_C_STANDARD 99)

include_directories(
        http/inc
        server_worker/inc
        utils/inc
)

add_executable(main

        main.c
        # SERVER_WORKER
        server_worker/src/request_pool_t.c
        server_worker/src/server_worker_t.c

        # HTTP
        http/src/http.c

        # UTILS
        utils/src/files.c
        server_worker/src/request_t.c
        logger/src/logger.c
)

target_compile_options(main PRIVATE -Wall -Wextra)

cmake_minimum_required(VERSION 3.22)
project(main C)

set(CMAKE_C_STANDARD 99)

add_executable(main main.c server_worker/src/server_worker_t.c)


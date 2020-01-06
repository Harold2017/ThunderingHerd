cmake_minimum_required(VERSION 3.15)
project(ThunderingHerd C)

set(CMAKE_C_STANDARD 11)

add_executable(ThunderingHerd epoll/main.c)
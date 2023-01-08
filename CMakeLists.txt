cmake_minimum_required(VERSION 3.22)
project(yahtcbot C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(yahtcbot
        test.c
        yahtcbot.c
        yahtcbot.h)

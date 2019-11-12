cmake_minimum_required(VERSION 3.15)
project(studentspar C)

set(CMAKE_C_STANDARD 99)

add_executable(studentspar studentspar.c)
target_link_libraries(studentspar m)
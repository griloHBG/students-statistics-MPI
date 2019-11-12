cmake_minimum_required(VERSION 3.15)
project(studentsseq C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")

add_executable(studentsseq studentsseq.c)
target_link_libraries(studentsseq m)
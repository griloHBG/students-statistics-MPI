cmake_minimum_required(VERSION 3.15)
project(studentsseq C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")

add_executable(studentsseq studentsseq.c)
target_link_libraries(studentsseq m)


project(old_studentsseq C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")

add_executable(old_studentsseq old_studentsseq.c)
target_link_libraries(old_studentsseq m)
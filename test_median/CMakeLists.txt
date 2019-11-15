cmake_minimum_required(VERSION 3.15)
project(test_median C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fopenmp")

add_executable(test_median test_median.c)
target_link_libraries(test_median m)
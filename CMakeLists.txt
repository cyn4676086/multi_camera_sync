cmake_minimum_required(VERSION 3.16)

project(infinite_sense VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -O3  -Wall")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3  -Wall")
endif ()

add_subdirectory(infinite_sense_core)

add_subdirectory(example)

cmake_minimum_required(VERSION 3.10)
project(serial VERSION 1.0)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(SRC_DIR .)

file(GLOB SOURCES ${SRC_DIR}/src/*.cc)
file(GLOB HEADERS ${SRC_DIR}/include/*.h)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${SRC_DIR}/include)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
    VERSION ${PROJECT_VERSION}
    LINK_FLAGS_RELEASE "-no-pie"
)
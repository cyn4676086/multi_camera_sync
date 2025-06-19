project(udp)
add_library(${PROJECT_NAME} STATIC src/practical_socket.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ./include)
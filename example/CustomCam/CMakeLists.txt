cmake_minimum_required(VERSION 3.16)

project(custom_cam VERSION 1.0)

add_executable(${PROJECT_NAME}
        main.cpp
        cus_cam.cpp
        cus_cam.h
)
target_link_directories(${PROJECT_NAME} PRIVATE
        # Add the path to the your cam SDK here
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        infinite_sense_core
        # Add the name of your cam SDK library here
)

target_include_directories(${PROJECT_NAME} PRIVATE
        # Add the path to the your cam SDK header files here
)
set_target_properties(${PROJECT_NAME} PROPERTIES
        INSTALL_RPATH "$ORIGIN"
)
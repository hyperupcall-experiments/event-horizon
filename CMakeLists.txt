cmake_minimum_required(VERSION 3.28)
project(event-horizon)

find_package(raylib REQUIRED)

add_executable(${PROJECT_NAME} ./src/launcher.c)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_INCLUDE_PATH})
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

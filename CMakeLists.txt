cmake_minimum_required(VERSION 3.28)
project(event-horizon, C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)

find_package(raylib REQUIRED)
find_package(libsystemd REQUIRED)
find_package(wayland REQUIRED)
add_compile_options(-g -Wall -Wextra -Wpedantic -Wno-sign-compare -fsanitize=address,undefined)
add_link_options(-fsanitize=address,undefined)

add_executable(keymon ./src/keymon.c)
target_include_directories(keymon PRIVATE ${CMAKE_INCLUDE_PATH})
target_link_libraries(keymon libsystemd::libsystemd)

add_executable(launcher ./src/launcher.c)
target_include_directories(launcher PRIVATE ${CMAKE_INCLUDE_PATH})
target_link_libraries(launcher raylib)

add_executable(wayland-test ./src/test.c)
target_include_directories(wayland-test PRIVATE ${CMAKE_INCLUDE_PATH})
target_link_libraries(wayland-test wayland::wayland)

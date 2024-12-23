cmake_minimum_required(VERSION 3.28)
project(event-horizon)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(raylib REQUIRED)
find_package(libsystemd REQUIRED)

add_executable(keymon ./src/keymon.c)
target_include_directories(keymon PRIVATE ${CMAKE_INCLUDE_PATH})
target_link_libraries(keymon libsystemd::libsystemd)

add_executable(launcher ./src/launcher.c)
target_include_directories(launcher PRIVATE ${CMAKE_INCLUDE_PATH})
target_link_libraries(launcher raylib)

# add_executable(wayland-test ./src/test.c)
# target_include_directories(wayland-test PRIVATE ${CMAKE_INCLUDE_PATH})
# target_link_libraries(wayland-test raylib libsystemd::libsystemd)

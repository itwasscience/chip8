cmake_minimum_required(VERSION 3.17)
project(chip8 C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(SDL2 REQUIRED)

include_directories(.)
include_directories(src)
include_directories(${SDL2_INCLUDE_DIR})

add_executable(chip8 main.c)

target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARY})

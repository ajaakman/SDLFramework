cmake_minimum_required(VERSION 3.14.0)

project(SDLFramework)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(Engine src/main.cpp)

target_link_libraries(Engine ${SDL2_LIBRARIES})
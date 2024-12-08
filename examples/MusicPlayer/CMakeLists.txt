cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -cppmeta")
include(FetchContent)

project(MusicPlayer)

add_subdirectory(../../ Spiral)
include_directories(../../)

add_executable(MusicPlayer main.cpp)

target_link_libraries(MusicPlayer Spiral)
cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_COMPILER "/Users/groundswell/Documents/GitHub/llvm-build-release/bin/clang++")
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -cppmeta")
include(FetchContent)

project(Spiral)


find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(./deps/glad/include)
include_directories(./deps/nanovg/src)
include_directories(./deps/nativefiledialog-extended/src/include)
include_directories(./)


add_executable(Spiral main.cpp mini_audio_impl.cpp)
target_sources(Spiral PRIVATE ./deps/glad/src/glad.c)
target_sources(Spiral PRIVATE ./deps/nanovg/src/nanovg.c)
add_subdirectory(./deps/nativefiledialog-extended)

target_link_libraries(Spiral ${SDL2_LIBRARIES})
target_link_libraries(Spiral nfd)
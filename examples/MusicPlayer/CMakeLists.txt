cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 23)
include(FetchContent)

project(MusicPlayer)

find_package(TagLib 2.0 REQUIRED)
find_package(ZLIB)

FetchContent_Declare(filesystem
  GIT_REPOSITORY https://github.com/gulrak/filesystem.git)
FetchContent_MakeAvailable(filesystem)

add_subdirectory(../../ Weave)
include_directories(../../)

include_directories(${TagLib_SOURCE_DIR})
include_directories(${filesystem_SOURCE_DIR}/include)

add_executable(MusicPlayer main.cpp)

target_include_directories(MusicPlayer PUBLIC ${TagLib_SOURCE_DIR})

target_link_libraries(MusicPlayer Weave TagLib::tag)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -cppmeta")
include(FetchContent)

project(Spiral)

FetchContent_Declare(SDL2
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-2.30.0)
FetchContent_MakeAvailable(SDL2)

FetchContent_Declare(nanovg
  GIT_REPOSITORY https://github.com/memononen/nanovg.git)
FetchContent_MakeAvailable(nanovg)

FetchContent_Declare(nfd 
  GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git)
FetchContent_MakeAvailable(nfd)

FetchContent_Declare(miniaudio 
 GIT_REPOSITORY https://github.com/mackron/miniaudio.git)
FetchContent_MakeAvailable(miniaudio)

FetchContent_Declare(dr_libs
 GIT_REPOSITORY https://github.com/mackron/dr_libs.git)
FetchContent_MakeAvailable(dr_libs)

FetchContent_Declare(stb
  GIT_REPOSITORY https://github.com/nothings/stb.git)
FetchContent_MakeAvailable(stb)

include_directories(./deps/glad/include)
include_directories(./)
include_directories(${SDL2_SOURCE_DIR}/include)
include_directories(${nanovg_SOURCE_DIR}/src)
include_directories(${nfd_SOURCE_DIR}/include)
include_directories(${miniaudio_SOURCE_DIR})
include_directories(${dr_libs_SOURCE_DIR})
include_directories(${stb_SOURCE_DIR})

if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  add_executable(Spiral main.cpp misc/mini_audio_impl.cpp graphics/graphics.cpp)
else()
  add_library(Spiral STATIC misc/mini_audio_impl.cpp graphics/graphics.cpp)
endif()

target_include_directories(Spiral PUBLIC 
  ./deps/glad/include
  ${SDL2_SOURCE_DIR}/include 
  ${nanovg_SOURCE_DIR}/src
  ${nfd_SOURCE_DIR}/include
  ${miniaudio_SOURCE_DIR}
  ${dr_libs_SOURCE_DIR}
  ${stb_SOURCE_DIR})

target_sources(Spiral PRIVATE ./deps/glad/src/glad.c)
target_sources(Spiral PRIVATE ${nanovg_SOURCE_DIR}/src/nanovg.c)

target_link_libraries(Spiral SDL2::SDL2main SDL2::SDL2-static)
target_link_libraries(Spiral nfd)
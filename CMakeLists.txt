cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 23)
include(FetchContent)

project(Weave)

FetchContent_Declare(swl-variant
  GIT_REPOSITORY https://github.com/groundswellaudio/swl-variant.git
  GIT_TAG "origin/main")
FetchContent_MakeAvailable(swl-variant)

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

FetchContent_Declare(kumi-tuple
  GIT_REPOSITORY https://github.com/jfalcou/kumi.git
  GIT_TAG "origin/main")

set(KUMI_BUILD_TEST OFF)
FetchContent_MakeAvailable(kumi-tuple)

include_directories(./deps/glad/include)
include_directories(./)
include_directories(${SDL2_SOURCE_DIR}/include)
include_directories(${nanovg_SOURCE_DIR}/src)
include_directories(${nfd_SOURCE_DIR}/include)
include_directories(${miniaudio_SOURCE_DIR})
include_directories(${dr_libs_SOURCE_DIR})
include_directories(${stb_SOURCE_DIR})

if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  add_executable(Weave main.cpp misc/mini_audio_impl.cpp graphics/graphics.cpp)
else()
  add_library(Weave STATIC misc/mini_audio_impl.cpp graphics/graphics.cpp)
endif()

target_include_directories(Weave PUBLIC 
  ./deps/glad/include
  ${SDL2_SOURCE_DIR}/include 
  ${nanovg_SOURCE_DIR}/src
  ${nfd_SOURCE_DIR}/include
  ${miniaudio_SOURCE_DIR}
  ${dr_libs_SOURCE_DIR}
  ${stb_SOURCE_DIR}
  ${swl-variant_SOURCE_DIR}/include
  ${kumi-tuple_SOURCE_DIR}/include)

target_sources(Weave PRIVATE ./deps/glad/src/glad.c)
target_sources(Weave PRIVATE ${nanovg_SOURCE_DIR}/src/nanovg.c)

target_link_libraries(Weave SDL2::SDL2main SDL2::SDL2-static)
target_link_libraries(Weave nfd)
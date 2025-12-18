cmake_minimum_required(VERSION 3.20)
project(CallOfGPT LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

option(BUILD_EXAMPLES "Build bundled examples" OFF)

FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.0
)

set(FETCHCONTENT_QUIET OFF)
FetchContent_MakeAvailable(raylib)

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
  "src/*.cpp"
)

add_executable(CallOfGPT ${GAME_SOURCES})
target_include_directories(CallOfGPT PRIVATE src)

if(MSVC)
  target_compile_options(CallOfGPT PRIVATE /W4 /permissive-)
else()
  target_compile_options(CallOfGPT PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(CallOfGPT PRIVATE raylib)

if(WIN32)
  # Statically link the runtime for a portable Windows binary.
  target_link_options(CallOfGPT PRIVATE /SUBSYSTEM:WINDOWS)
endif()

if(APPLE)
  # Required frameworks for raylib on macOS.
  target_link_libraries(CallOfGPT PRIVATE
    -framework IOKit
    -framework Cocoa
    -framework OpenGL
  )
endif()

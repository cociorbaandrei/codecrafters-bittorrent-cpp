cmake_minimum_required(VERSION 3.14)
project(bittorrent-starter-cpp)
set(CMAKE_CXX_STANDARD 20) # Enable the C++23 standard

find_package(libuv QUIET) # Attempt to find libuv
if(NOT libuv_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    libuv
    GIT_REPOSITORY https://github.com/libuv/libuv.git
    GIT_TAG v1.47.0 # Specify the desired version of libuv
  )
  FetchContent_MakeAvailable(libuv)
endif()

# Fetch uvw
include(FetchContent)
FetchContent_Declare(
  uvw
  GIT_REPOSITORY https://github.com/skypjack/uvw.git
  GIT_TAG v3.3.0_libuv_v1.47 # Specify the desired version of uvw
)
FetchContent_MakeAvailable(uvw)

set(SOURCE_FILES src/Main.cpp src/lib/nlohmann/json.hpp)
add_executable(bittorrent ${SOURCE_FILES})
target_link_libraries(bittorrent PRIVATE uv_a uvw::uvw) # Link against libuv
target_include_directories(bittorrent PUBLIC ${uvw_SOURCE_DIR}/src) # Include uvw headers
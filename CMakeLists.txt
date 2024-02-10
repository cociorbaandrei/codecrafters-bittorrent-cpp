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

FetchContent_Declare(
  fmtlib
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        8.0.1 # Specify a tag or commit hash here for reproducibility
)

FetchContent_MakeAvailable(fmtlib)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.9.2 # It's a good practice to pin a specific version for reproducibility
)

FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
  indicators
  GIT_REPOSITORY https://github.com/p-ranav/indicators.git
  GIT_TAG        master # You can specify a specific tag or commit if needed
)

FetchContent_MakeAvailable(indicators)


set(SOURCE_FILES src/Main.cpp src/lib/nlohmann/json.hpp src/utils.cpp src/FileManager.cpp)
add_executable(bittorrent ${SOURCE_FILES} "src/BTConnection.cpp" "src/BitTorrentMessage.cpp" "src/old_utils.cpp")
target_link_libraries(bittorrent PRIVATE uv_a uvw::uvw fmt spdlog::spdlog indicators) # Link against libuv
target_include_directories(bittorrent PUBLIC ${uvw_SOURCE_DIR}/src ${spdlog_SOURCE_DIR}/src) # Include uvw headers
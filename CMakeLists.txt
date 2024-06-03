cmake_minimum_required(VERSION 3.14)
project(bittorrent-starter-cpp)
set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard

Set(FETCHCONTENT_QUIET FALSE)
find_package(libuv QUIET) # Attempt to find libuv
# Download CPM.cmake                                                                                                                             
file(                                                                                                                                            
  DOWNLOAD      https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.38.3/CPM.cmake ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake           
  EXPECTED_HASH SHA256=cc155ce02e7945e7b8967ddfaff0b050e958a723ef7aad3766d368940cb15494                                                          
)                                                                                                                                                
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

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

# Fetch the cpp-httplib library
FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.10.8  # Specify the version tag you want to use
)

FetchContent_MakeAvailable(httplib)

set(TRY_BOOST_VERSION "1.85.0")
set(BOOST_NOT_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED "thread;cobalt")
set(BOOST_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED "asio;beast;filesystem;cobalt")


set(BOOST_INCLUDE_LIBRARIES
    "${BOOST_NOT_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED};${BOOST_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED}"
)

# url for 1.85.0 + only
set(BOOST_URL
    "https://github.com/boostorg/boost/releases/download/boost-${TRY_BOOST_VERSION}/boost-${TRY_BOOST_VERSION}-cmake.tar.xz"
)
CPMAddPackage(
  NAME Boost
  URL ${BOOST_URL}
  OPTIONS "BOOST_SKIP_INSTALL_RULES OFF"
)

find_package(OpenSSL)

# Define BOOST_INCLUDE_DIR
set(BOOST_INCLUDE_DIR ${boost_SOURCE_DIR})

# Add Boost headers
include_directories(${BOOST_INCLUDE_DIR})


# Add the main library target
add_library(torrett_lib
    ${PROJECT_SOURCE_DIR}/src/utils.cpp
    ${PROJECT_SOURCE_DIR}/src/FileManager.cpp
    ${PROJECT_SOURCE_DIR}/src/BitTorrentMessage.cpp
    ${PROJECT_SOURCE_DIR}/src/BTConnection.cpp
    ${PROJECT_SOURCE_DIR}/src/old_utils.cpp
    ${PROJECT_SOURCE_DIR}/src/HttpClient.cpp
    ${PROJECT_SOURCE_DIR}/src/FileReader.cpp
    ${PROJECT_SOURCE_DIR}/src/TrackerService.cpp
    ${PROJECT_SOURCE_DIR}/src/lib/nlohmann/json.hpp
)

# Include directories for the test executable
target_include_directories(torrett_lib PUBLIC
    ${uvw_SOURCE_DIR}/src
    ${spdlog_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(torrett_lib uv_a spdlog::spdlog indicators httplib boost_beast boost_asio boost_filesystem  boost_cobalt) 

set(SOURCE_FILES src/Main.cpp)
add_executable(bittorrent ${SOURCE_FILES})
target_link_libraries(bittorrent 
  PRIVATE 
  uv_a 
  uvw::uvw 
  fmt 
  spdlog::spdlog 
  indicators 
  torrett_lib
  boost_filesystem
  boost_cobalt
  Boost::cobalt OpenSSL::SSL
) # Link against libuv
target_include_directories(bittorrent PUBLIC ${uvw_SOURCE_DIR}/src ${spdlog_SOURCE_DIR}/src ${Boost_INCLUDE_DIRS}) # Include uvw headers

foreach(a_lib ${BOOST_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED})
target_link_libraries(bittorrent PUBLIC boost_${a_lib})
endforeach()

foreach(a_lib ${BOOST_NOT_HEADER_ONLY_COMPONENTS_THAT_YOU_NEED})
target_link_libraries(bittorrent PUBLIC Boost::${a_lib})
endforeach()

if (UNIT_TESTS)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
  set(BUILD_GTEST ON CACHE BOOL "" FORCE)

  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.8.0
  )

  FetchContent_MakeAvailable(googletest)

  enable_testing()
  add_subdirectory(tests)
endif()
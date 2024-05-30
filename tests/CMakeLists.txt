include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.8.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(BUILD_GTEST ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)
enable_testing()
target_compile_features(gmock PRIVATE cxx_std_20)
target_compile_features(gmock_main PRIVATE cxx_std_20)
find_package(libuv QUIET) # Attempt to find libuv

################################
# Tests
################################
# Add test cpp file

# Add the test executable
add_executable(unit-tests test.cpp)
# Add the main library target
add_library(my_library
    ${PROJECT_SOURCE_DIR}/src/utils.cpp
)

# Include directories for the test executable
target_include_directories(my_library PUBLIC
    ${spdlog_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(my_library spdlog::spdlog)
# Link the test executable against gmock and gtest libraries
target_link_libraries(unit-tests gmock gtest gtest_main my_library spdlog::spdlog uv_a uvw::uvw)

# Set the C++ standard for the test executable
set_target_properties(unit-tests PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES)
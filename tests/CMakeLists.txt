include(FetchContent)
enable_testing()
find_package(libuv QUIET) # Attempt to find libuv

################################
# Tests
################################
# Add test cpp file

# Add the test executable
add_executable(unit-tests test.cpp)

target_link_libraries(unit-tests gmock gtest gtest_main torrett_lib)

# Set the C++ standard for the test executable
set_target_properties(unit-tests PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
include(FetchContent)

find_package(libuv QUIET) # Attempt to find libuv

set(TARGET bittorent_unit_test)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/unit-test)

add_executable(
    ${TARGET}
    bittorent_unit_test.cpp
)

target_include_directories(
    ${TARGET}
    PRIVATE ${CMAKE_SOURCE_DIR}
            ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(
    ${TARGET}
    PRIVATE gmock gtest gtest_main torrett_lib
)

target_compile_options(${TARGET} PRIVATE -Wno-gnu-zero-variadic-macro-arguments)

add_test(NAME bittorent_unit_test COMMAND bittorent_unit_test
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/unit-test/
)

#install(TARGETS ${TARGET} DESTINATION ${CMAKE_BINARY_DIR}/unit-test/bittorent_unit_test)

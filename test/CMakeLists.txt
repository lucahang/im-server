cmake_minimum_required(VERSION 3.16)

find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

add_executable(im_server_test
    net/codec_test.cpp
)

target_include_directories(im_server_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(im_server_test PRIVATE
    GTest::gtest_main
    Threads::Threads
)

include(GoogleTest)
gtest_discover_tests(im_server_test)

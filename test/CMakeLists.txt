cmake_minimum_required(VERSION 3.16)

find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

add_executable(im_server_test
    net/codec_test.cpp
    net/network_integration_test.cpp
)

target_include_directories(im_server_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(im_server_test PRIVATE
    GTest::gtest_main
    Boost::system
    Threads::Threads
)

add_executable(tcp_benchmark
    benchmark/tcp_benchmark.cpp
)

target_link_libraries(tcp_benchmark PRIVATE
    Boost::system
    Threads::Threads
)

add_executable(full_im_benchmark
    benchmark/full_im_benchmark.cpp
)

target_link_libraries(full_im_benchmark PRIVATE
    Boost::system
    Threads::Threads
)

add_executable(latency_test
    benchmark/latency_test.cpp
)

target_link_libraries(latency_test PRIVATE
    Boost::system
    Threads::Threads
)

add_executable(concurrency_test
    benchmark/concurrency_test.cpp
)

target_link_libraries(concurrency_test PRIVATE
    Boost::system
    Threads::Threads
)

include(GoogleTest)
gtest_discover_tests(im_server_test)

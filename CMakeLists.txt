cmake_minimum_required(VERSION 3.16)
project(im-server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_options(-Wall -Wextra -O2)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Protobuf REQUIRED)
find_package(spdlog REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(redis++ REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQL REQUIRED IMPORTED_TARGET mysqlclient)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS proto/message.proto)

set(SOURCES
    src/main.cpp
    src/net/codec.cpp
    src/net/connection.cpp
    src/net/tcp_server.cpp
    src/db/database.cpp
    src/db/redis_wrapper.cpp
    src/business/user_manager.cpp
    src/business/message_handler.cpp
    src/business/msg_manager.cpp
    ${PROTO_SRCS}
)

add_executable(im_server ${SOURCES})

target_include_directories(im_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(im_server PRIVATE
    Boost::system
    spdlog::spdlog
    protobuf::libprotobuf
    OpenSSL::Crypto
    PkgConfig::MYSQL
    redis++::redis++
    hiredis
    Threads::Threads
)

enable_testing()
add_subdirectory(test)

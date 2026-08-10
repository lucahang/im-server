cmake_minimum_required(VERSION 3.16)
project(im-server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(redis++ REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQL REQUIRED IMPORTED_TARGET mysqlclient)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS proto/message.proto)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(SOURCES
    src/main.cpp
    src/codec.cpp
    src/connection.cpp
    src/tcp_server.cpp
    src/user_manager.cpp
    src/message_handler.cpp
    src/database.cpp
    src/redis_wrapper.cpp
    src/msg_manager.cpp
    ${PROTO_SRCS}
)

add_executable(im_server ${SOURCES})
target_link_libraries(im_server
    Boost::system
    protobuf::libprotobuf
    OpenSSL::Crypto
    PkgConfig::MYSQL
    redis++
    hiredis
    pthread
)
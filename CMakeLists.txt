cmake_minimum_required(VERSION 3.16)
project(im-server LANGUAGES CXX)

# ==============================================================================
# 1. C++ 编译器选项与标准设置
# ==============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # 禁用编译器特定扩展，确保代码符合严格的 C++17 标准

# 设置默认编译类型为 Debug（如果不指定的话）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# 开启常用编译警告，协助检测潜在逻辑问题
add_compile_options(-Wall -Wextra -O2)

# ==============================================================================
# 2. 寻找依赖库 (Dependencies)
# ==============================================================================
find_package(Boost REQUIRED COMPONENTS system)
find_package(Protobuf REQUIRED)
find_package(spdlog REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(redis++ REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQL REQUIRED IMPORTED_TARGET mysqlclient)

# Find Threads 自动匹配当前平台的最佳线程库 (替代硬编码 -lpthread)
find_package(Threads REQUIRED)

# ==============================================================================
# 3. Protobuf 代码生成
# ==============================================================================
# 自动根据 proto/message.proto 生成 message.pb.cc 和 message.pb.h
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS proto/message.proto)

# ==============================================================================
# 4. 源文件列表 (与镜像子目录匹配)
# ==============================================================================
set(SOURCES
    src/main.cpp

    # 网络通信模块 (Network Layer)
    src/net/codec.cpp
    src/net/connection.cpp
    src/net/tcp_server.cpp

    # 存储持久化模块 (Database & Cache Layer)
    src/db/database.cpp
    src/db/redis_wrapper.cpp

    # 核心业务逻辑模块 (Business Layer)
    src/business/user_manager.cpp
    src/business/message_handler.cpp
    src/business/msg_manager.cpp

    # 包含 Protobuf 自动生成的源码
    ${PROTO_SRCS}
)

# ==============================================================================
# 5. 可执行文件目标配置 (Target Configurations)
# ==============================================================================
add_executable(im_server ${SOURCES})

# 设置头文件包含目录
target_include_directories(im_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR} # 供包含在 build 目录下生成的 message.pb.h
)

# 链接依赖库
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
# IM Server

基于 **C++17** 开发的即时通讯系统服务端，面向 Linux 环境，采用 Boost.Asio 提供 TCP 异步网络通信，并结合 Protocol Buffers、MySQL、Redis 和 spdlog 完成协议、持久化、缓存与日志等基础能力。

本项目与 [im-client](https://github.com/lucahang/im-client) 配套使用，主要用于实践 C++ 网络编程、异步 IO、客户端/服务端协议设计、数据库访问以及 IM 业务开发。

## Features

- 用户注册与登录
- TCP 长连接通信
- Boost.Asio 异步 IO
- Protobuf 序列化通信协议
- 单聊消息
- 群聊消息协议
- 历史消息查询与分页加载
- 未读消息清理
- 联系人列表
- 好友申请、同意/拒绝好友申请
- 删除好友
- 心跳与退出流程
- MySQL 数据持久化
- Redis 缓存支持
- spdlog 日志

## Architecture

```text
                    +-------------------+
                    |    IM Client      |
                    +---------+---------+
                              |
                         TCP Connection
                              |
                    +---------v---------+
                    |    TCP Server     |
                    +---------+---------+
                              |
              +---------------+---------------+
              |                               |
      +-------v-------+               +-------v-------+
      | Network Layer |               | Business Layer|
      |               |               |               |
      | Connection    |               | User Manager  |
      | TCP Server    |               | Msg Manager   |
      | Codec         |               | Msg Handler   |
      +-------+-------+               +-------+-------+
              |                               |
              +---------------+---------------+
                              |
                    +---------v---------+
                    | Storage / Cache   |
                    |                   |
                    | MySQL + Redis     |
                    +-------------------+
```

### Network Layer

核心网络代码位于 `src/net`：

- `tcp_server.cpp`：TCP 服务端监听与连接管理
- `connection.cpp`：客户端连接及异步读写
- `codec.cpp`：消息编解码

客户端与服务端通过 TCP 长连接通信。消息使用 Protobuf 序列化，并在网络层增加长度前缀，用于解决 TCP 字节流中的消息边界问题。

### Business Layer

核心业务代码位于 `src/business`：

- `user_manager.cpp`：用户相关业务
- `message_handler.cpp`：消息请求处理与业务分发
- `msg_manager.cpp`：消息相关管理

### Database & Cache

`src/db` 提供数据存储相关能力：

- MySQL：业务数据持久化
- Redis：缓存及高频访问数据

### Protocol

协议定义位于 `proto/message.proto`，通过 Protobuf 自动生成 C++ 代码。

当前协议覆盖：

```text
Register / Login
Single Message / Group Message
Get History / Load More History
Clear Unread
Get Contacts
Add Friend
Get Friend Requests
Response to Friend Request
Delete Friend
Heartbeat / Quit
```

每条消息包含统一的 `Header`，其中包括 `cmd`、`seq` 和 `status`，业务数据放在 `body` 中。

## Project Structure

```text
im-server/
├── CMakeLists.txt
├── config/                 # 配置文件
├── include/
│   ├── net/                # 网络层头文件
│   ├── db/                 # 数据库/缓存头文件
│   └── business/           # 业务层头文件
├── src/
│   ├── main.cpp
│   ├── net/
│   ├── db/
│   └── business/
├── proto/
│   └── message.proto       # Protobuf 协议定义
└── logs/                   # 日志目录
```

## Tech Stack

| 技术 | 用途 |
| --- | --- |
| C++17 | 核心开发语言 |
| Boost.Asio | TCP 异步网络通信 |
| Protocol Buffers | 网络消息序列化 |
| MySQL | 关系型数据持久化 |
| Redis / redis++ | 缓存与高频数据访问 |
| spdlog | 日志系统 |
| OpenSSL | 加密相关依赖 |
| CMake | 项目构建 |
| pthread | 多线程支持 |

## Build

### Environment

建议使用 Linux 环境，并准备以下依赖：

- GCC / G++，支持 C++17
- CMake 3.16+
- Boost
- Protobuf
- spdlog
- OpenSSL
- MySQL client
- Redis++ / hiredis

### Compile

```bash
git clone https://github.com/lucahang/im-server.git
cd im-server

mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

生成的服务端可执行文件为：

```text
im_server
```

> 运行前需要根据本地环境配置 MySQL、Redis 以及服务端相关配置。

## Client

对应的 Qt/QML 客户端：

https://github.com/lucahang/im-client

## Development Goals

- [ ] 完善连接管理与异常处理
- [ ] 完善 Redis 缓存策略
- [ ] 离线消息与可靠投递
- [ ] 消息重试与 ACK 机制
- [ ] 更完善的心跳/超时检测
- [ ] 服务端性能测试与压测
- [ ] 多实例部署与服务端集群
- [ ] Docker 化部署

## Author

**luca jay**

C++ / Qt Developer
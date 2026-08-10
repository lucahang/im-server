#pragma once
#include <string>
#include <optional>
#include "message.pb.h"

class Codec {
public:
    // 将 Message 编码为【4字节网络序长度 + 序列化数据】
    static std::string Encode(const im::Message& msg);

    // 从完整 Body 数据反序列化 Message
    static std::optional<im::Message> Decode(const char* data, size_t len);
};
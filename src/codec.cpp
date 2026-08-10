#include "codec.h"
#include <arpa/inet.h>
#include <cstring>

std::string Codec::Encode(const im::Message& msg) {
    std::string body = msg.SerializeAsString(); //将 Protobuf 对象序列化为二进制字节流（
    int32_t netLen = htonl(static_cast<int32_t>(body.size()));
    std::string result;
    result.reserve(4 + body.size());
    result.append(reinterpret_cast<const char*>(&netLen), 4);
    result.append(body);
    return result;
}

std::optional<im::Message> Codec::Decode(const char* data, size_t len) {
    im::Message msg;
    if (!msg.ParseFromArray(data, static_cast<int>(len))) {
        return std::nullopt;
    }
    return msg;
}
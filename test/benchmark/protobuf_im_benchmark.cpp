#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>

// This benchmark uses generated protobuf messages when message.pb.h exists.
// It validates serialization overhead and network sending path.

#include "message.pb.h"

using boost::asio::ip::tcp;

struct ProtoBenchmarkResult {
    int messages = 0;
    size_t bytes = 0;
    long long elapsed_us = 0;
};

ProtoBenchmarkResult RunProtoBenchmark(
        const std::string& host,
        int port,
        int count)
{
    boost::asio::io_context io;
    tcp::socket socket(io);

    boost::system::error_code ec;
    socket.connect(
        tcp::endpoint(boost::asio::ip::make_address(host), port),
        ec);

    ProtoBenchmarkResult result;

    if (ec) {
        return result;
    }

    auto begin = std::chrono::steady_clock::now();

    for (int i = 0; i < count; ++i) {
        im::Message packet;
        auto header = packet.mutable_header();
        header->set_cmd(im::CMD_SINGLE_MSG);
        header->set_seq(i);

        im::ChatMessage chat;
        chat.set_sender("10001");
        chat.set_receiver("10002");
        chat.set_content("benchmark message");

        std::string body;
        chat.SerializeToString(&body);
        packet.set_body(body);

        std::string data;
        packet.SerializeToString(&data);

        boost::asio::write(socket,
                           boost::asio::buffer(data),
                           ec);

        if (!ec) {
            result.messages++;
            result.bytes += data.size();
        }
    }

    auto end = std::chrono::steady_clock::now();
    result.elapsed_us =
        std::chrono::duration_cast<std::chrono::microseconds>(
            end - begin).count();

    return result;
}

int main()
{
    auto result = RunProtoBenchmark(
        "127.0.0.1",
        8080,
        10000);

    std::cout << "protobuf messages: "
              << result.messages << std::endl;
    std::cout << "bytes: "
              << result.bytes << std::endl;
    std::cout << "time(us): "
              << result.elapsed_us << std::endl;

    if (result.elapsed_us > 0) {
        std::cout << "QPS: "
                  << result.messages * 1000000 / result.elapsed_us
                  << std::endl;
    }
}

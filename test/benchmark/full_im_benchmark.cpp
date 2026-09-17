#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <memory>

using boost::asio::ip::tcp;

struct IMBenchmarkResult {
    int login_requests{0};
    int message_requests{0};
    int success_connections{0};
    long long elapsed_ms{0};
};

// Full IM business benchmark skeleton.
// Flow:
// connect -> login request -> send message
//
// The packet generation can be replaced by protobuf Message
// after generated message.pb.h is included.
IMBenchmarkResult RunIMBenchmark(
        const std::string& host,
        int port,
        int users,
        int messages)
{
    boost::asio::io_context io;
    std::vector<std::unique_ptr<tcp::socket>> clients;

    IMBenchmarkResult result;

    auto begin = std::chrono::steady_clock::now();

    for (int i = 0; i < users; ++i) {
        auto socket = std::make_unique<tcp::socket>(io);
        boost::system::error_code ec;

        socket->connect(
            tcp::endpoint(boost::asio::ip::make_address(host), port),
            ec);

        if (!ec) {
            result.success_connections++;
            clients.emplace_back(std::move(socket));
        }
    }

    std::string login = "LOGIN_REQUEST";
    std::string message = "SINGLE_MESSAGE";

    for (auto& client : clients) {
        boost::system::error_code ec;
        boost::asio::write(*client,
                           boost::asio::buffer(login),
                           ec);
        if (!ec) {
            result.login_requests++;
        }
    }

    for (auto& client : clients) {
        for (int i = 0; i < messages; ++i) {
            boost::system::error_code ec;
            boost::asio::write(*client,
                               boost::asio::buffer(message),
                               ec);
            if (!ec) {
                result.message_requests++;
            }
        }
    }

    auto end = std::chrono::steady_clock::now();
    result.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            end - begin).count();

    return result;
}

int main()
{
    auto result = RunIMBenchmark(
        "127.0.0.1",
        8080,
        100,
        100);

    std::cout << "===== Full IM Benchmark =====\n";
    std::cout << "online users: " << result.success_connections << "\n";
    std::cout << "login requests: " << result.login_requests << "\n";
    std::cout << "message requests: " << result.message_requests << "\n";
    std::cout << "elapsed(ms): " << result.elapsed_ms << "\n";

    if (result.elapsed_ms > 0) {
        std::cout << "message QPS: "
                  << result.message_requests * 1000 / result.elapsed_ms
                  << "\n";
    }
}

#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <memory>

using boost::asio::ip::tcp;

struct BenchmarkResult {
    int success_connections{0};
    int failed_connections{0};
    int total_messages{0};
    long long elapsed_ms{0};
};

// IM protocol pressure benchmark.
// Current version focuses on TCP transport layer.
// It can be extended with protobuf login/send-message packets later.
BenchmarkResult RunBenchmark(
        const std::string& host,
        int port,
        int clients,
        int messages_per_client)
{
    boost::asio::io_context io;
    std::vector<std::unique_ptr<tcp::socket>> sockets;

    BenchmarkResult result;
    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < clients; ++i) {
        auto socket = std::make_unique<tcp::socket>(io);
        boost::system::error_code ec;

        socket->connect(
            tcp::endpoint(boost::asio::ip::make_address(host), port), ec);

        if (ec) {
            result.failed_connections++;
            continue;
        }

        result.success_connections++;
        sockets.emplace_back(std::move(socket));
    }

    const std::string message = "benchmark message";

    for (auto& socket : sockets) {
        for (int i = 0; i < messages_per_client; ++i) {
            boost::system::error_code ec;
            boost::asio::write(
                *socket,
                boost::asio::buffer(message),
                ec);

            if (!ec) {
                result.total_messages++;
            }
        }
    }

    auto end = std::chrono::steady_clock::now();
    result.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();

    return result;
}

int main()
{
    constexpr int clients = 100;
    constexpr int messages = 100;

    auto result = RunBenchmark(
        "127.0.0.1",
        8080,
        clients,
        messages);

    std::cout << "========== IM Server Benchmark ==========" << std::endl;
    std::cout << "connections success: "
              << result.success_connections << std::endl;
    std::cout << "connections failed: "
              << result.failed_connections << std::endl;
    std::cout << "messages sent: "
              << result.total_messages << std::endl;
    std::cout << "elapsed(ms): "
              << result.elapsed_ms << std::endl;

    if (result.elapsed_ms > 0) {
        std::cout << "QPS: "
                  << result.total_messages * 1000 / result.elapsed_ms
                  << std::endl;
    }
}

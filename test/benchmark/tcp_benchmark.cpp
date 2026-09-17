#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

using boost::asio::ip::tcp;

int main()
{
    constexpr int client_count = 100;
    constexpr int message_count = 100;

    boost::asio::io_context io;
    std::vector<std::unique_ptr<tcp::socket>> clients;

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < client_count; ++i) {
        auto socket = std::make_unique<tcp::socket>(io);
        boost::system::error_code ec;
        socket->connect(
            tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), 8080), ec);

        if (!ec)
            clients.emplace_back(std::move(socket));
    }

    std::string payload = "benchmark message";
    size_t total = 0;

    for (auto& client : clients) {
        for (int i = 0; i < message_count; ++i) {
            boost::system::error_code ec;
            boost::asio::write(*client, boost::asio::buffer(payload), ec);
            if (!ec)
                total++;
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();

    std::cout << "clients: " << clients.size() << std::endl;
    std::cout << "messages: " << total << std::endl;
    std::cout << "time(ms): " << ms << std::endl;
    std::cout << "QPS: " << (total * 1000.0 / ms) << std::endl;

    return 0;
}

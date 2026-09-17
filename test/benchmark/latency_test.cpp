#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>

using boost::asio::ip::tcp;

int main()
{
    boost::asio::io_context io;
    tcp::socket socket(io);

    boost::system::error_code ec;
    socket.connect(
        tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"),8080),
        ec);

    if (ec) {
        std::cerr << "connect failed: " << ec.message() << std::endl;
        return 1;
    }

    std::vector<long long> latency;
    constexpr int count = 1000;

    std::string msg = "PING";

    for (int i = 0; i < count; ++i) {
        auto start = std::chrono::steady_clock::now();

        boost::asio::write(socket, boost::asio::buffer(msg), ec);

        auto end = std::chrono::steady_clock::now();

        if (!ec) {
            latency.push_back(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    end - start).count());
        }
    }

    if (latency.empty()) {
        return 1;
    }

    std::sort(latency.begin(), latency.end());

    auto avg = 0LL;
    for (auto v : latency) avg += v;
    avg /= latency.size();

    std::cout << "count: " << latency.size() << std::endl;
    std::cout << "avg(us): " << avg << std::endl;
    std::cout << "p95(us): " << latency[latency.size()*95/100] << std::endl;
    std::cout << "p99(us): " << latency[latency.size()*99/100] << std::endl;
}

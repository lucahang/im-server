#include <boost/asio.hpp>
#include <thread>
#include <vector>
#include <iostream>
#include <atomic>

using boost::asio::ip::tcp;

int main()
{
    constexpr int clients = 1000;
    std::atomic<int> success{0};

    std::vector<std::thread> threads;
    threads.reserve(clients);

    for (int i = 0; i < clients; ++i) {
        threads.emplace_back([&]() {
            try {
                boost::asio::io_context io;
                tcp::socket socket(io);

                socket.connect(
                    tcp::endpoint(
                        boost::asio::ip::make_address("127.0.0.1"),
                        8080));

                success++;

                std::this_thread::sleep_for(
                    std::chrono::seconds(10));
            }
            catch (...) {
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    std::cout << "target clients: " << clients << std::endl;
    std::cout << "connected clients: " << success.load() << std::endl;
}

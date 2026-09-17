#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <thread>
#include <chrono>

using boost::asio::ip::tcp;

// Integration test: verify TCP port can accept client connections.
// Start im_server before running this test.
TEST(NetworkIntegrationTest, TcpConnectionCanBeEstablished)
{
    boost::asio::io_context io;
    tcp::socket socket(io);

    tcp::endpoint endpoint(boost::asio::ip::make_address("127.0.0.1"), 8080);

    boost::system::error_code ec;
    socket.connect(endpoint, ec);

    EXPECT_FALSE(ec) << "connect failed: " << ec.message();
}

TEST(NetworkIntegrationTest, MultipleClientsCanConnect)
{
    boost::asio::io_context io;

    constexpr int client_count = 20;
    std::vector<std::unique_ptr<tcp::socket>> clients;

    for (int i = 0; i < client_count; ++i) {
        auto socket = std::make_unique<tcp::socket>(io);
        boost::system::error_code ec;
        socket->connect(
            tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), 8080), ec);
        EXPECT_FALSE(ec);
        clients.emplace_back(std::move(socket));
    }
}

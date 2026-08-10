#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <thread>
#include <vector>
#include "tcp_server.h"
#include "user_manager.h"
#include "message_handler.h"
#include "database.h"
#include "redis_wrapper.h"
#include "msg_manager.h"

int main() {
    try {
        boost::asio::io_context ioc;
        auto work = boost::asio::make_work_guard(ioc);

        // 修改为你的 MySQL 参数
        Database db("localhost", "root", "159751", "im_db");
        RedisClient redis;
        MsgManager msgManager(redis);

        UserManager userManager;
        MessageHandler msgHandler(userManager, db, msgManager);

        TcpServer server(ioc, 9000, userManager, msgHandler);
        server.Start();

        unsigned int threadCount = std::thread::hardware_concurrency();
        if (threadCount == 0) threadCount = 4;
        std::vector<std::thread> threads;
        for (unsigned int i = 0; i < threadCount; ++i)
            threads.emplace_back([&ioc] { ioc.run(); });

        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&](const boost::system::error_code& ec, int) {
            if (!ec) {
                std::cout << "\nShutting down..." << std::endl;
                ioc.stop();
            }
        });

        for (auto& t : threads)
            if (t.joinable()) t.join();

        std::cout << "Server exited cleanly." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
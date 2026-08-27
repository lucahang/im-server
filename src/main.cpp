#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <thread>
#include <vector>
#include <filesystem>

#include "net/tcp_server.h"
#include "business/user_manager.h"
#include "business/message_handler.h"
#include "db/database.h"
#include "db/redis_wrapper.h"
#include "business/msg_manager.h"

void InitCombinedLogger() {
    std::filesystem::path current_p = std::filesystem::current_path();
    current_p=current_p.parent_path();
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    std::string log_path=current_p.string()+"/logs/im_server.log";
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path, false);

    std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink};
    auto combined_logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());
    
    // 🌟 1. 设置最低日志级别（建议设置为 debug 或 info）
    combined_logger->set_level(spdlog::level::debug);
    
    // 🌟 2. 设置日志格式：[时间] [日志级别] [线程ID] 内容
    combined_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");

    spdlog::set_default_logger(combined_logger);
    spdlog::flush_every(std::chrono::seconds(1));
    // 🌟 3. 遇到 error 级别的日志时立即刷新磁盘，防止程序挂掉丢失关键错误日志
    spdlog::flush_on(spdlog::level::err);
}

int main() {
    try {
        InitCombinedLogger();
        boost::asio::io_context ioc;
        auto work = boost::asio::make_work_guard(ioc);

        // 修改为你的 MySQL 参数
        Database db("localhost", "root", "159751", "im_db");
        RedisClient redis("tcp://127.0.0.1:6379");
        MsgManager msgManager(redis, db);

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
                // std::cout << "\nShutting down..." << std::endl;
                spdlog::info("\nShutting down...");
                ioc.stop();
            }
        });

        for (auto& t : threads)
            if (t.joinable()) t.join();

        // std::cout << "Server exited cleanly." << std::endl;
        spdlog::info("Server exited cleanly." );
        spdlog::shutdown(); 
    } catch (const std::exception& e) {
        // std::cerr << "Fatal error: " << e.what() << std::endl;
        spdlog::error("Fatal error: {}",e.what());
        return 1;
    }
    return 0;
}



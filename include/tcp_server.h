#pragma once
#include <boost/asio.hpp>
#include <memory>

class UserManager;
class MessageHandler;

class TcpServer {
public:
    TcpServer(boost::asio::io_context& ioc, uint16_t port,
              UserManager& userManager, MessageHandler& msgHandler);

    // 开始监听并接受连接
    void Start();

private:
    void DoAccept();

    boost::asio::ip::tcp::acceptor acceptor_;
    UserManager& userManager_;
    MessageHandler& msgHandler_;
};
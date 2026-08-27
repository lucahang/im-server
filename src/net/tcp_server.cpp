#include "net/tcp_server.h"
#include "net/connection.h"

#include <iostream>
#include <spdlog/spdlog.h>

TcpServer::TcpServer(boost::asio::io_context& ioc, uint16_t port,
                     UserManager& userManager, MessageHandler& msgHandler)
    : acceptor_(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
    , userManager_(userManager)
    , msgHandler_(msgHandler) {}

void TcpServer::Start() {
    // std::cout << "Server listening on port " << acceptor_.local_endpoint().port() << std::endl;
    spdlog::info("Server listening on port {}",acceptor_.local_endpoint().port());
    DoAccept();
}

void TcpServer::DoAccept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                std::make_shared<Connection>(std::move(socket), userManager_, msgHandler_)->Start();
            }
            DoAccept();
        });
}
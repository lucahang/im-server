#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <deque>
#include <mutex>
#include <optional>
#include "message.pb.h"

class UserManager;
class MessageHandler;

class Connection : public std::enable_shared_from_this<Connection> {
public:
    Connection(boost::asio::ip::tcp::socket socket,
               UserManager& userManager,
               MessageHandler& msgHandler);

    void Start();
    void Send(const im::Message& msg);
    void SetUserId(const std::string& uid);
    std::optional<std::string> GetUserId() const;
    void Close();

private:
    void AsyncReadLength();
    void AsyncReadBody(int32_t bodyLen);
    void OnMessageReceived(const im::Message& msg);
    void DoWrite();

    boost::asio::ip::tcp::socket socket_;
    UserManager& userManager_;
    MessageHandler& msgHandler_;

    std::array<char, 4> lengthBuffer_{};
    std::vector<char> bodyBuffer_;

    std::optional<std::string> userId_;

    std::deque<std::string> sendQueue_;
    std::mutex sendMutex_;
    bool writing_ = false;
};
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

    void Start();                              // 开始读取长度头
    void Send(const im::Message& msg);         // 异步发送（线程安全）
    void SetUserId(const std::string& uid);    // 绑定用户 ID
    std::optional<std::string> GetUserId() const;
    void Close();

private:
    void AsyncReadLength();                    // 读 4 字节长度
    void AsyncReadBody(int32_t bodyLen);       // 读 Body
    void OnMessageReceived(const im::Message& msg); // 完整消息回调
    void DoWrite();                            // 从发送队列中取数据异步写

    boost::asio::ip::tcp::socket socket_;
    UserManager& userManager_;
    MessageHandler& msgHandler_;

    std::array<char, 4> lengthBuffer_{};
    std::vector<char> bodyBuffer_;

    std::optional<std::string> userId_;

    // 发送队列（串行化异步写）
    std::deque<std::string> sendQueue_;
    std::mutex sendMutex_;
    bool writing_ = false;
};
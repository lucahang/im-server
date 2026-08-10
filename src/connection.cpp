#include "connection.h"
#include "codec.h"
#include "user_manager.h"
#include "message_handler.h"
#include <iostream>

Connection::Connection(boost::asio::ip::tcp::socket socket,
                       UserManager& userManager,
                       MessageHandler& msgHandler)
    : socket_(std::move(socket))
    , userManager_(userManager)
    , msgHandler_(msgHandler) {}

void Connection::Start() {
    AsyncReadLength();
}

void Connection::SetUserId(const std::string& uid) {
    userId_ = uid;
}

std::optional<std::string> Connection::GetUserId() const {
    return userId_;
}

void Connection::Send(const im::Message& msg) {
    std::string data = Codec::Encode(msg);
    {
        std::lock_guard<std::mutex> lock(sendMutex_);
        sendQueue_.push_back(std::move(data));
        if (!writing_) {
            writing_ = true;
            // 在 socket 线程中启动写，确保线程安全
            boost::asio::post(socket_.get_executor(),
                [self = shared_from_this()] { self->DoWrite(); });
        }
    }
}

void Connection::AsyncReadLength() {
    auto self = shared_from_this();
    boost::asio::async_read(socket_, boost::asio::buffer(lengthBuffer_),
        [this, self](boost::system::error_code ec, std::size_t /*len*/) {
            if (ec) {
                Close();
                return;
            }
            int32_t bodyLen = 0;
            std::memcpy(&bodyLen, lengthBuffer_.data(), 4);
            bodyLen = ntohl(bodyLen);
            if (bodyLen <= 0 || bodyLen > 1024 * 1024) { // 限制最大 1MB
                Close();
                return;
            }
            AsyncReadBody(bodyLen);
        });
}

void Connection::AsyncReadBody(int32_t bodyLen) {
    auto self = shared_from_this();
    bodyBuffer_.resize(bodyLen);
    boost::asio::async_read(socket_, boost::asio::buffer(bodyBuffer_),
        [this, self](boost::system::error_code ec, std::size_t /*len*/) {
            if (ec) {
                Close();
                return;
            }
            auto msg = Codec::Decode(bodyBuffer_.data(), bodyBuffer_.size());
            if (msg) {
                OnMessageReceived(*msg);
            }
            // 继续读取下一条消息的长度头
            AsyncReadLength();
        });
}

void Connection::OnMessageReceived(const im::Message& msg) {
    // 交给业务分发器
    msgHandler_.OnMessage(shared_from_this(), msg);
}

void Connection::DoWrite() {
    std::string data;
    {
        std::lock_guard<std::mutex> lock(sendMutex_);
        if (sendQueue_.empty()) {
            writing_ = false;
            return;
        }
        data = std::move(sendQueue_.front());
        sendQueue_.pop_front();
    }
    auto self = shared_from_this();
    boost::asio::async_write(socket_, boost::asio::buffer(data),
        [this, self](boost::system::error_code ec, std::size_t /*len*/) {
            if (ec) {
                Close();
                return;
            }
            DoWrite();  // 发送队列中的下一条
        });
}

void Connection::Close() {
    boost::system::error_code ec;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    socket_.close(ec);
    // 清理用户管理器中的记录
    if (userId_) {
        userManager_.RemoveUser(*userId_);
    }
}
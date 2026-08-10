#pragma once
#include "message.pb.h"
#include <memory>

class Connection;
class UserManager;
class Database;

class MessageHandler {
public:
    MessageHandler(UserManager& userManager, Database& db);

    void OnMessage(std::shared_ptr<Connection> conn, const im::Message& msg);

private:
    void HandleRegisterReq(std::shared_ptr<Connection> conn, const im::Message& msg);
    void HandleLoginReq(std::shared_ptr<Connection> conn, const im::Message& msg);
    void HandleChatReq(std::shared_ptr<Connection> conn, const im::Message& msg);
    void HandleHeartbeat(std::shared_ptr<Connection> conn, const im::Message& msg);

    UserManager& userManager_;
    Database& db_;
};
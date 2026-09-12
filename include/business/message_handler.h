#pragma once
#include "message.pb.h"
#include <memory>

class Connection;
class UserManager;
class Database;
class MsgManager;

class MessageHandler {
public:
    MessageHandler(UserManager& userManager, Database& db, MsgManager& msgManager);

    void OnMessage(std::shared_ptr<Connection> conn, const im::Message& msg);

private:
    void HandleRegisterReq(std::shared_ptr<Connection> conn, const im::Message& msg);
    void HandleLoginReq(std::shared_ptr<Connection> conn, const im::Message& msg);
    void HandleQuitReq(std::shared_ptr<Connection> conn, const im::Message& msg);
    void HandleSingleMsg(std::shared_ptr<Connection> conn, const im::Message& msg);
    void HandleGroupMsg(std::shared_ptr<Connection> conn, const im::Message& msg);
    void HandleGetHistory(std::shared_ptr<Connection> conn, const im::Message& msg);
    void HandleDeleteFriendReq(std::shared_ptr<Connection> conn, const im::Message& msg);
    void HandleGetContactsRep(std::shared_ptr<Connection> conn, const im::Message& msg);
    void HandleAddFriendReq(std::shared_ptr<Connection> conn, const im::Message& msg);
    void HandleGetFriendReqs(std::shared_ptr<Connection> conn, const im::Message& msg);
    void HandleClearUnread(std::shared_ptr<Connection> conn, const im::Message& msg);
    void HandleHeartbeat(std::shared_ptr<Connection> conn, const im::Message& msg);
    void HandleResponeToFriendReqs(std::shared_ptr<Connection> conn, const im::Message& msg);

    UserManager& userManager_;
    Database& db_;
    MsgManager& msgManager_;
};
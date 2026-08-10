#include "message_handler.h"
#include "connection.h"
#include "user_manager.h"
#include "database.h"
#include "security.h"
#include <iostream>

MessageHandler::MessageHandler(UserManager& userManager, Database& db)
    : userManager_(userManager), db_(db) {}

void MessageHandler::OnMessage(std::shared_ptr<Connection> conn, const im::Message& msg) {
    switch (msg.header().cmd()) {
        case im::CMD_REGISTER_REQ:
            HandleRegisterReq(conn, msg);
            break;
        case im::CMD_LOGIN_REQ:
            HandleLoginReq(conn, msg);
            break;
        case im::CMD_CHAT_REQ:
            HandleChatReq(conn, msg);
            break;
        case im::CMD_HEARTBEAT:
            HandleHeartbeat(conn, msg);
            break;
        default:
            // echo unknown
            conn->Send(msg);
            break;
    }
}

void MessageHandler::HandleRegisterReq(std::shared_ptr<Connection> conn, const im::Message& msg) {
    im::RegisterRequest req;
    if (!req.ParseFromString(msg.body())) return;

    std::string username = req.username();
    std::string password = req.password();
    if (username.empty() || password.empty()) {
        im::Message respMsg;
        respMsg.mutable_header()->set_cmd(im::CMD_REGISTER_RES);
        respMsg.mutable_header()->set_seq(msg.header().seq());
        respMsg.mutable_header()->set_status(1); // 参数错误
        im::RegisterResponse resp;
        resp.set_status(1);
        respMsg.set_body(resp.SerializeAsString());
        conn->Send(respMsg);
        return;
    }

    // 生成盐和哈希
    std::string salt = GenerateSalt();
    std::string hash = SHA256Hash(password + salt);

    int ret = db_.RegisterUser(username, hash, salt);
    int status = (ret == 0) ? 0 : ((ret == -1) ? 2 : 3); // 0成功，2用户名重复，3其他错误

    im::Message respMsg;
    respMsg.mutable_header()->set_cmd(im::CMD_REGISTER_RES);
    respMsg.mutable_header()->set_seq(msg.header().seq());
    respMsg.mutable_header()->set_status(status);
    im::RegisterResponse resp;
    resp.set_status(status);
    respMsg.set_body(resp.SerializeAsString());
    conn->Send(respMsg);

    std::cout << "Register " << username << " status: " << status << std::endl;
}

void MessageHandler::HandleLoginReq(std::shared_ptr<Connection> conn, const im::Message& msg) {
    im::LoginRequest req;
    if (!req.ParseFromString(msg.body())) return;

    std::string username = req.username();
    std::string password = req.password();

    im::Message respMsg;
    respMsg.mutable_header()->set_cmd(im::CMD_LOGIN_RES);
    respMsg.mutable_header()->set_seq(msg.header().seq());

    int userid = -1;
    std::string db_salt, db_hash;
    if (db_.GetUserInfo(username, db_salt, db_hash, userid)) {
        std::string input_hash = SHA256Hash(password + db_salt);
        if (input_hash == db_hash) {
            // 登录成功
            std::string uid_str = std::to_string(userid);
            conn->SetUserId(uid_str);
            userManager_.AddUser(uid_str, conn);

            respMsg.mutable_header()->set_status(0);
            im::LoginResponse resp;
            resp.set_status(0);
            resp.set_user_id(uid_str);
            respMsg.set_body(resp.SerializeAsString());
            std::cout << "User login: " << username << " (id=" << uid_str << ")" << std::endl;
        } else {
            // 密码错误
            respMsg.mutable_header()->set_status(1);
            im::LoginResponse resp;
            resp.set_status(1);
            respMsg.set_body(resp.SerializeAsString());
            std::cout << "Login failed (bad password): " << username << std::endl;
        }
    } else {
        // 用户不存在
        respMsg.mutable_header()->set_status(2);
        im::LoginResponse resp;
        resp.set_status(2);
        respMsg.set_body(resp.SerializeAsString());
        std::cout << "Login failed (no user): " << username << std::endl;
    }

    conn->Send(respMsg);
}

void MessageHandler::HandleChatReq(std::shared_ptr<Connection> conn, const im::Message& msg) {
    im::ChatMessage chat;
    if (!chat.ParseFromString(msg.body())) return;

    // 发送者必须已登录，从连接获取 user_id
    auto sender_id = conn->GetUserId();
    if (!sender_id) return;

    // 发送回执
    im::Message ack;
    ack.mutable_header()->set_cmd(im::CMD_CHAT_RES);
    ack.mutable_header()->set_seq(msg.header().seq());
    ack.mutable_header()->set_status(0);
    conn->Send(ack);

    // 转发给接收者
    auto receiverConn = userManager_.GetUser(chat.receiver());
    if (receiverConn) {
        im::Message forwardMsg;
        forwardMsg.mutable_header()->set_cmd(im::CMD_CHAT_REQ);
        forwardMsg.mutable_header()->set_seq(msg.header().seq());
        forwardMsg.set_body(msg.body());
        receiverConn->Send(forwardMsg);
    } else {
        im::Message offlineAck;
        offlineAck.mutable_header()->set_cmd(im::CMD_CHAT_RES);
        offlineAck.mutable_header()->set_seq(msg.header().seq());
        offlineAck.mutable_header()->set_status(1001); // 接收者离线
        conn->Send(offlineAck);
    }
}

void MessageHandler::HandleHeartbeat(std::shared_ptr<Connection> conn, const im::Message& msg) {
    conn->Send(msg); // echo
}
#include <spdlog/spdlog.h>
#include <iostream>

#include "business/message_handler.h"
#include "net/connection.h"
#include "business/user_manager.h"
#include "db/database.h"
#include "business/msg_manager.h"
#include "utils/security.h"

MessageHandler::MessageHandler(UserManager& userManager, Database& db, MsgManager& msgManager)
    : userManager_(userManager), db_(db), msgManager_(msgManager) {}

void MessageHandler::OnMessage(std::shared_ptr<Connection> conn, const im::Message& msg) {
    int cmd = msg.header().cmd();
    switch (cmd) {
        case im::CMD_REGISTER_REQ:       HandleRegisterReq(conn, msg); break;
        case im::CMD_LOGIN_REQ:          HandleLoginReq(conn, msg); break;
        case im::CMD_GET_CONTACTS_REQ:   HandleGetContactsRep(conn, msg); break;
        case im::CMD_QUIT_REQ:           HandleQuitReq(conn, msg);break; 
        case im::CMD_SINGLE_MSG:         HandleSingleMsg(conn, msg); break;
        case im::CMD_GROUP_MSG:          HandleGroupMsg(conn, msg); break;
        case im::CMD_GET_HISTORY_REQ:    HandleGetHistory(conn, msg); break;
        case im::CMD_ADD_FRIEND_REQ:     HandleAddFriendReq(conn,msg); break;
        case im::CMD_GET_FRIEND_REQS_REQ:HandleGetFriendReqs(conn,msg); break;
        case im::CMD_RESPONE_TO_FRIEND_REQS_REQ: HandleResponeToFriendReqs(conn,msg); break;
        case im::CMD_CLEAR_UNREAD_REQ:   HandleClearUnread(conn, msg); break;
        case im::CMD_HEARTBEAT:          HandleHeartbeat(conn, msg); break;
        default: conn->Send(msg); break; // echo
    }
}

void MessageHandler::HandleGetFriendReqs(std::shared_ptr<Connection> conn, const im::Message& msg){
    auto userId = conn->GetUserId();
    if (!userId) return;

    spdlog::info("userId: {} request friend requests",userId.value());
    auto friendReqs = db_.GetFriendRequests(std::stoll(userId.value()));

    im::Message respMsg;
    respMsg.mutable_header()->set_cmd(im::CMD_GET_FRIEND_REQS_RES);
    respMsg.mutable_header()->set_seq(msg.header().seq());
    respMsg.mutable_header()->set_status(0);

    im::GetFriendRequestsResponse resp;
    resp.set_status(0);

    for (auto& fr : friendReqs) {
        //spdlog::debug("{}",fr.DebugString());
        *resp.add_requests() = fr;
    }

    respMsg.set_body(resp.SerializeAsString());
    conn->Send(respMsg);

}

void MessageHandler::HandleGetContactsRep(std::shared_ptr<Connection> conn, const im::Message& msg) {
    auto userId = conn->GetUserId();
    if (!userId) return;
    
    im::HistoryRequest req;
    if (!req.ParseFromString(msg.body())) return;
    spdlog::info("userId: {} request contact lists",userId.value());
    auto contacts = db_.GetUserContacts(userId.value());

    im::Message respMsg;
    respMsg.mutable_header()->set_cmd(im::CMD_GET_CONTACTS_RES);
    respMsg.mutable_header()->set_seq(msg.header().seq());
    respMsg.mutable_header()->set_status(0);

    im::ContactResponse resp;
    resp.set_status(0);
    for (auto& c : contacts) {
        //spdlog::debug("{}",c.DebugString());
        *resp.add_contacts() = c;
    }
    respMsg.set_body(resp.SerializeAsString());
    conn->Send(respMsg);
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
        respMsg.mutable_header()->set_status(1);
        im::RegisterResponse resp;
        resp.set_status(1);
        respMsg.set_body(resp.SerializeAsString());
        conn->Send(respMsg);
        return;
    }

    std::string salt = GenerateSalt();
    std::string hash = SHA256Hash(password + salt);
    int ret = db_.RegisterUser(username, hash, salt);
    int status = (ret == 0) ? 0 : ((ret == -1) ? 2 : 3);

    im::Message respMsg;
    respMsg.mutable_header()->set_cmd(im::CMD_REGISTER_RES);
    respMsg.mutable_header()->set_seq(msg.header().seq());
    respMsg.mutable_header()->set_status(status);
    im::RegisterResponse resp;
    resp.set_status(status);
    respMsg.set_body(resp.SerializeAsString());
    conn->Send(respMsg);
    // std::cout << "Register " << username << " status: " << status << std::endl;
    spdlog::info("Register {} status: {}",username,status);
}

void MessageHandler::HandleResponeToFriendReqs(std::shared_ptr<Connection> conn, const im::Message& msg){
    im::ResponseToFriendReqsReq req;
    if (!req.ParseFromString(msg.body())) return;

    int64_t user_id = std::stoll(req.user_id());
    int64_t peer_id = std::stoll(req.peer_id());
    int32_t status = req.status();
    //spdlog::debug("user_id: {}, peer_id:{}, status: {}",user_id, peer_id, status);
    if(db_.UpdateRequestStatus(peer_id, user_id, status)){
        if(status == 1){
            std::string username = db_.GetUserName(user_id);
            std::string peername = db_.GetUserName(peer_id);

            db_.InsertFriendship(peer_id, user_id, username, 1);
            db_.InsertFriendship(user_id, peer_id, peername, 1);

            spdlog::info("{} and {} are friend now!", user_id, peer_id);
            std::string peer_id_str = std::to_string(peer_id);
            if(userManager_.FindUser(peer_id_str)){
                auto peer=userManager_.GetUser(peer_id_str);

                auto contacts = db_.GetUserContacts(peer_id_str);

                im::Message respMsg;
                respMsg.mutable_header()->set_cmd(im::CMD_GET_CONTACTS_RES);
                respMsg.mutable_header()->set_seq(msg.header().seq());
                respMsg.mutable_header()->set_status(0);

                im::ContactResponse resp;
                resp.set_status(0);
                for (auto& c : contacts) {
                    //spdlog::debug("{}",c.DebugString());
                    *resp.add_contacts() = c;
                }
                respMsg.set_body(resp.SerializeAsString());
                peer->Send(respMsg);
            }
        }
    }
    else{
        spdlog::warn("UpdateRequestStatus({},{},{}) error",peer_id, user_id, status);
    }

    im::Message respMsg;
    respMsg.mutable_header()->set_cmd(im::CMD_RESPONE_TO_FRIEND_REQS_RES);
    respMsg.mutable_header()->set_seq(msg.header().seq());
    respMsg.mutable_header()->set_status(0);
    im::ResponseToFriendReqsRes resp;
    resp.set_status(status);
    respMsg.set_body(resp.SerializeAsString());
    conn->Send(respMsg);
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
            std::string uid_str = std::to_string(userid);
            conn->SetUserId(uid_str);
            userManager_.AddUser(uid_str, conn);

            respMsg.mutable_header()->set_status(0);
            im::LoginResponse resp;
            resp.set_status(0);
            resp.set_user_id(uid_str);
            resp.set_username(username);
            respMsg.set_body(resp.SerializeAsString());
            // std::cout << "Login: " << username << " (id=" << uid_str << ")" << std::endl;
            spdlog::info("Login: {} (id={})",username,uid_str);
        } else {
            respMsg.mutable_header()->set_status(1);
            im::LoginResponse resp;
            resp.set_status(1);
            respMsg.set_body(resp.SerializeAsString());
            // std::cout << "Login failed (bad pw): " << username << std::endl;
            spdlog::info("Login failed (bad pw): {}",username);
        }
    } else {
        respMsg.mutable_header()->set_status(2);
        im::LoginResponse resp;
        resp.set_status(2);
        respMsg.set_body(resp.SerializeAsString());
        // std::cout << "Login failed (no user): " << username << std::endl;
        spdlog::info("Login failed (no user): {}",username);
    }
    conn->Send(respMsg);
}

void MessageHandler::HandleQuitReq(std::shared_ptr<Connection> conn, const im::Message& msg) {
    std::string user_id=msg.body();
    spdlog::info("user_id: {} disconnected",user_id);
    userManager_.RemoveUser(user_id);
}

void MessageHandler::HandleAddFriendReq(std::shared_ptr<Connection> conn, const im::Message& msg){
    im::AddFriendRequest addFriRes;
    if (!addFriRes.ParseFromString(msg.body())) return;

    std::string user_id = addFriRes.from_user_id();
    std::string from_user_name = addFriRes.from_user_name();
    std::string target_name = addFriRes.to_user_name();
    std::string m_msg = addFriRes.message();

    //find username id
    int target_id = -1;
    std::string db_salt, db_hash;

    im::Message respMsg;
    respMsg.mutable_header()->set_cmd(im::CMD_ADD_FRIEND_RES);
    respMsg.mutable_header()->set_seq(msg.header().seq());

    if (db_.GetUserInfo(target_name, db_salt, db_hash, target_id)) {
        im::AddFriendResponse resp;
        if(target_id == std::stoi(user_id)){
            resp.set_status(3);
            respMsg.set_body(resp.SerializeAsString());
            conn->Send(respMsg);
            return ;
        }
        // 已经发过了FriendRequest
        if(db_.ExistFriendRequest(std::stoll(user_id), static_cast<long long>(target_id))){
            resp.set_status(1);
            respMsg.set_body(resp.SerializeAsString());
            conn->Send(respMsg);
            return ;
        }
        resp.set_status(0);
        //spdlog::debug("target_name: {}",target_name);
        resp.set_msg(target_name);
        respMsg.set_body(resp.SerializeAsString());
        db_.InsertAddFriendReq(user_id, std::to_string(target_id), from_user_name, m_msg);
        conn->Send(respMsg);
        spdlog::info("user_id-{} successfully sent a friend message to {}",user_id,target_name);
    }
    else{
        respMsg.mutable_header()->set_status(2);
        im::AddFriendResponse resp;
        resp.set_status(2);
        respMsg.set_body(resp.SerializeAsString());
        conn->Send(respMsg);
        spdlog::info("there is not user called {}",target_name);
    }
    
}

void MessageHandler::HandleSingleMsg(std::shared_ptr<Connection> conn, const im::Message& msg) {
    auto userId = conn->GetUserId();
    if (!userId) return;

    im::ChatMessage chat;
    if (!chat.ParseFromString(msg.body())) return;

    chat.set_sender(*userId);
    if (chat.receiver().empty()) return;
    
    auto peer_id = chat.receiver();
    if(userManager_.FindUser(peer_id)){
        auto peer=userManager_.GetUser(peer_id);
        peer->Send(msg);
    }

    msgManager_.SendSingleMsg(chat);
}

void MessageHandler::HandleGroupMsg(std::shared_ptr<Connection> conn, const im::Message& msg) {
    auto userId = conn->GetUserId();
    if (!userId) return;

    im::ChatMessage chat;
    if (!chat.ParseFromString(msg.body())) return;
    chat.set_sender(*userId);
    if (chat.group_id().empty()) return;

    // 群成员应动态查询，此处演示固定群组 "test_group"
    std::vector<std::string> members = {"1", "2", "3"};
    msgManager_.SendGroupMsg(chat, members);
}

void MessageHandler::HandleGetHistory(std::shared_ptr<Connection> conn, const im::Message& msg) {
    auto userId = conn->GetUserId();
    if (!userId) return;

    im::HistoryRequest req;
    if (!req.ParseFromString(msg.body())) return;

    auto msgs = msgManager_.GetHistory(*userId, req.peer_id(), req.is_group(),
                                       req.start(), req.count());

    im::Message respMsg;
    if(req.count() == 20){
        respMsg.mutable_header()->set_cmd(im::CMD_GET_HISTORY_RES);
    }
    else if(req.count() == 10){
        respMsg.mutable_header()->set_cmd(im::CMD_GET_LOADMORE_HISTORY_RES);
    }
    respMsg.mutable_header()->set_seq(msg.header().seq());
    respMsg.mutable_header()->set_status(0);

    im::HistoryResponse resp;
    resp.set_status(0);
    for (auto& m : msgs) {
        *resp.add_messages() = m;
    }
    respMsg.set_body(resp.SerializeAsString());
    conn->Send(respMsg);
}

void MessageHandler::HandleClearUnread(std::shared_ptr<Connection> conn, const im::Message& msg) {
    auto userId = conn->GetUserId();
    if (!userId) return;

    im::ClearUnreadRequest req;
    if (!req.ParseFromString(msg.body())) return;

    msgManager_.ClearUnread(*userId, req.peer_id(), req.is_group());

    im::Message respMsg;
    respMsg.mutable_header()->set_cmd(im::CMD_CLEAR_UNREAD_RES);
    respMsg.mutable_header()->set_seq(msg.header().seq());
    respMsg.mutable_header()->set_status(0);
    im::ClearUnreadResponse resp;
    resp.set_status(0);
    respMsg.set_body(resp.SerializeAsString());
    conn->Send(respMsg);
}

void MessageHandler::HandleHeartbeat(std::shared_ptr<Connection> conn, const im::Message& msg) {
    conn->Send(msg);
}
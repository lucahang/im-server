#pragma once
#include "db/redis_wrapper.h"
#include "message.pb.h"
#include "db/database.h"
#include <string>
#include <vector>
#include <cstdint>

class MsgManager {
public:
    explicit MsgManager(RedisClient& redis, Database& db);

    void SendSingleMsg(const im::ChatMessage& chat);
    void SendGroupMsg(const im::ChatMessage& chat, const std::vector<std::string>& member_ids);
    std::vector<im::HistoryMessage> GetHistory(const std::string& uid,
                                               const std::string& peer_id,
                                               bool is_group,
                                               int64_t start, int32_t count);
    void ClearUnread(const std::string& uid, const std::string& peer_id, bool is_group);
    static std::string MakeSingleSessionId(const std::string& uid1, const std::string& uid2);
    static std::string MakeGroupSessionId(const std::string& group_id);
    int64_t NextMsgId();

private:
    void SaveMessage(const im::HistoryMessage& msg);
    void PushToQueue(const std::string& queue_key, int64_t msg_id);
    void UpdateUserSession(const std::string& uid, const std::string& session_id,
                           int64_t timestamp, const std::string& last_msg, bool is_sender);

    RedisClient& redis_;
    Database& db_;
};
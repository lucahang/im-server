#include "msg_manager.h"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <future>

using namespace sw::redis;

MsgManager::MsgManager(RedisClient& redis) : redis_(redis) {}

int64_t MsgManager::NextMsgId() {
    return redis_->incr("global:msgid");
}

std::string MsgManager::MakeSingleSessionId(const std::string& uid1, const std::string& uid2) {
    if (uid1 < uid2) return "single:" + uid1 + ":" + uid2;
    return "single:" + uid2 + ":" + uid1;
}

std::string MsgManager::MakeGroupSessionId(const std::string& group_id) {
    return "group:" + group_id;
}

void MsgManager::SaveMessage(const im::HistoryMessage& msg) {
    std::string key = "msg:" + std::to_string(msg.msg_id());
    redis_->hmset(key, {
        std::make_pair("sender", msg.sender()),
        std::make_pair("receiver", msg.receiver()),
        std::make_pair("group_id", msg.group_id()),
        std::make_pair("content", msg.content()),
        std::make_pair("timestamp", std::to_string(msg.timestamp()))
    });
}

void MsgManager::PushToQueue(const std::string& queue_key, int64_t msg_id) {
    redis_->lpush(queue_key, std::to_string(msg_id));
    redis_->ltrim(queue_key, 0, 999);
}

void MsgManager::UpdateUserSession(const std::string& uid,
                                   const std::string& session_id,
                                   int64_t timestamp,
                                   const std::string& last_msg,
                                   bool is_sender) {
    redis_->zadd("user:" + uid + ":sessions", session_id, timestamp);
    std::string detail_key = "user:" + uid + ":session:" + session_id;
    auto pipe = redis_->pipeline();
    if (!is_sender) {
        pipe.hincrby(detail_key, "unread", 1);
    }
    pipe.hset(detail_key, "last_msg", last_msg);
    pipe.exec();
}

void MsgManager::SendSingleMsg(const im::ChatMessage& chat) {
    int64_t msg_id = NextMsgId();
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    im::HistoryMessage hist_msg;
    hist_msg.set_msg_id(msg_id);
    hist_msg.set_sender(chat.sender());
    hist_msg.set_receiver(chat.receiver());
    hist_msg.set_group_id("");
    hist_msg.set_content(chat.content());
    hist_msg.set_timestamp(now);

    SaveMessage(hist_msg);

    std::string session_id = MakeSingleSessionId(chat.sender(), chat.receiver());
    PushToQueue("user:" + chat.sender() + ":queue:" + session_id, msg_id);
    PushToQueue("user:" + chat.receiver() + ":queue:" + session_id, msg_id);

    std::string last_msg = chat.content().substr(0, 30);
    UpdateUserSession(chat.receiver(), session_id, now, last_msg, false);
    UpdateUserSession(chat.sender(), session_id, now, last_msg, true);
}

void MsgManager::SendGroupMsg(const im::ChatMessage& chat, const std::vector<std::string>& member_ids) {
    int64_t msg_id = NextMsgId();
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    im::HistoryMessage hist_msg;
    hist_msg.set_msg_id(msg_id);
    hist_msg.set_sender(chat.sender());
    hist_msg.set_receiver("");
    hist_msg.set_group_id(chat.group_id());
    hist_msg.set_content(chat.content());
    hist_msg.set_timestamp(now);

    SaveMessage(hist_msg);

    std::string session_id = MakeGroupSessionId(chat.group_id());
    PushToQueue("group:" + chat.group_id() + ":queue", msg_id);

    std::string last_msg = chat.content().substr(0, 30);
    for (const auto& uid : member_ids) {
        bool is_sender = (uid == chat.sender());
        UpdateUserSession(uid, session_id, now, last_msg, is_sender);
    }
}

std::vector<im::HistoryMessage> MsgManager::GetHistory(const std::string& uid,
                                                       const std::string& peer_id,
                                                       bool is_group,
                                                       int64_t start, int32_t count) {
    std::string queue_key;
    if (is_group) {
        queue_key = "group:" + peer_id + ":queue";
    } else {
        std::string session_id = MakeSingleSessionId(uid, peer_id);
        queue_key = "user:" + uid + ":queue:" + session_id;
    }

    std::vector<std::string> msg_ids;
    redis_->lrange(queue_key, start, start + count - 1, std::back_inserter(msg_ids));

    std::vector<im::HistoryMessage> history;
    if (msg_ids.empty()) {
        return history;
    }

    // 1. 创建 Pipeline
    auto pipe = redis_->pipeline();

    // 2. 批量将 hgetall 加入管道
    for (const auto& id : msg_ids) {
        pipe.hgetall("msg:" + id);
    }

    // 3. 执行管道，获取 QueuedReplies 结果集
    sw::redis::QueuedReplies replies = pipe.exec();

    // 4. 遍历 replies 取出每个 hgetall 返回的 Hash Map
    for (std::size_t i = 0; i < replies.size(); ++i) {
        // 从 QueuedReplies 中按照索引提取映射字典
        using HashResult = std::unordered_map<std::string, std::string>;
        auto map = replies.get<HashResult>(i);

        if (map.empty()) {
            continue; // 忽略不存在的 msg key
        }

        im::HistoryMessage m;

        // 安全读取 map 的 Lambda
        auto get_val = [&map](const std::string& key) -> std::string {
            auto it = map.find(key);
            return (it != map.end()) ? it->second : "";
        };

        m.set_sender(get_val("sender"));
        m.set_receiver(get_val("receiver"));
        m.set_group_id(get_val("group_id"));
        m.set_content(get_val("content"));

        std::string ts_str = get_val("timestamp");
        if (!ts_str.empty()) {
            try {
                m.set_timestamp(std::stoll(ts_str));
            } catch (const std::exception&) {
                m.set_timestamp(0);
            }
        }

        history.push_back(m);
    }

    return history;
}


void MsgManager::ClearUnread(const std::string& uid, const std::string& peer_id, bool is_group) {
    std::string session_id = is_group ? MakeGroupSessionId(peer_id) : MakeSingleSessionId(uid, peer_id);
    redis_->hset("user:" + uid + ":session:" + session_id, "unread", "0");
}
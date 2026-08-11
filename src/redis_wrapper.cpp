#include "redis_wrapper.h"
#include "msg_manager.h"
RedisClient::RedisClient(const std::string& uri) {
    sw::redis::ConnectionOptions opts;
    opts.host = "127.0.0.1";
    opts.port = 6379;
    // opts.password = "your_password"; // 如需密码
    sw::redis::ConnectionPoolOptions pool_opts;
    pool_opts.size = 10;
    redis_ = std::make_shared<sw::redis::Redis>(opts, pool_opts);
}

void RedisClient::SaveMessage(const im::HistoryMessage& msg){
    std::string key = "msg:" + std::to_string(msg.msg_id());
    redis_->hmset(key, {
        std::make_pair("sender", msg.sender()),
        std::make_pair("receiver", msg.receiver()),
        std::make_pair("group_id", msg.group_id()),
        std::make_pair("content", msg.content()),
        std::make_pair("timestamp", std::to_string(msg.timestamp()))
    });
}

std::vector<im::HistoryMessage> RedisClient::GetMessagesBySession(const std::string& uid,
                                                       const std::string& peer_id,
                                                       bool is_group,
                                                       int64_t start, int32_t count){
    std::string queue_key;
    if (is_group) {
        queue_key = "group:" + peer_id + ":queue";
    } else {
        std::string session_id = MsgManager::MakeSingleSessionId(uid, peer_id);
        queue_key = "user:" + uid + ":queue:" + session_id;
    }

    // 1. 先查 Redis
    std::vector<std::string> msg_ids;
    redis_->lrange(queue_key, start, start + count - 1, std::back_inserter(msg_ids));

    std::vector<im::HistoryMessage> history;
    if (!msg_ids.empty()) {
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
    }
    return history;
}
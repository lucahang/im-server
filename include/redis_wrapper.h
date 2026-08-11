#pragma once
#include "message.pb.h"

#include <sw/redis++/redis++.h>
#include <string>
#include <memory>
#include <vector>

class RedisClient {
public:
    RedisClient(const std::string& uri = "tcp://127.0.0.1:6379");
    sw::redis::Redis& operator*() { return *redis_; }
    sw::redis::Redis* operator->() { return redis_.get(); }
    void SaveMessage(const im::HistoryMessage& msg);
    std::vector<im::HistoryMessage> GetMessagesBySession(const std::string& uid,
                                                       const std::string& peer_id,
                                                       bool is_group,
                                                       int64_t start, int32_t count);
private:
    std::shared_ptr<sw::redis::Redis> redis_;
};
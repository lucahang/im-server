#pragma once
#include <sw/redis++/redis++.h>
#include <string>
#include <memory>

class RedisClient {
public:
    RedisClient(const std::string& uri = "tcp://127.0.0.1:6379");
    sw::redis::Redis& operator*() { return *redis_; }
    sw::redis::Redis* operator->() { return redis_.get(); }

private:
    std::shared_ptr<sw::redis::Redis> redis_;
};
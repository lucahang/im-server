#include "redis_wrapper.h"

RedisClient::RedisClient(const std::string& uri) {
    sw::redis::ConnectionOptions opts;
    opts.host = "127.0.0.1";
    opts.port = 6379;
    // opts.password = "your_password"; // 如需密码
    sw::redis::ConnectionPoolOptions pool_opts;
    pool_opts.size = 10;
    redis_ = std::make_shared<sw::redis::Redis>(opts, pool_opts);
}
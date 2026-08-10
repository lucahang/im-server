#pragma once
#include <string>
#include <memory>
#include <mysql/mysql.h>

class Database {
public:
    Database(const std::string& host, const std::string& user,
             const std::string& pass, const std::string& db, int port = 3306);
    ~Database();

    // 禁止拷贝
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // 注册新用户，返回 0 表示成功，-1 用户名重复，-2 其他错误
    int RegisterUser(const std::string& username, const std::string& password_hash, const std::string& salt);

    // 查询用户信息，返回 true 并填充 salt 和 hash
    bool GetUserInfo(const std::string& username, std::string& out_salt, std::string& out_hash, int& out_userid);

private:
    MYSQL* conn_;
};
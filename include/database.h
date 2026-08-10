#pragma once
#include <string>
#include <mysql/mysql.h>

class Database {
public:
    Database(const std::string& host, const std::string& user,
             const std::string& pass, const std::string& db, int port = 3306);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    int RegisterUser(const std::string& username, const std::string& password_hash, const std::string& salt);
    bool GetUserInfo(const std::string& username, std::string& out_salt, std::string& out_hash, int& out_userid);

private:
    MYSQL* conn_;
};
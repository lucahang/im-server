#include "database.h"
#include <stdexcept>
#include <cstring>
#include <iostream>

Database::Database(const std::string& host, const std::string& user,
                   const std::string& pass, const std::string& db, int port) {
    conn_ = mysql_init(nullptr);
    if (!conn_) throw std::runtime_error("mysql_init failed");

    if (!mysql_real_connect(conn_, host.c_str(), user.c_str(), pass.c_str(),
                            db.c_str(), port, nullptr, 0)) {
        std::string err = mysql_error(conn_);
        mysql_close(conn_);
        throw std::runtime_error("mysql_real_connect failed: " + err);
    }
    mysql_set_character_set(conn_, "utf8mb4");
}

Database::~Database() {
    if (conn_) mysql_close(conn_);
}

int Database::RegisterUser(const std::string& username, const std::string& password_hash, const std::string& salt) {
    // 检查用户名是否已存在
    MYSQL_STMT* stmt = mysql_stmt_init(conn_);
    if (!stmt) return -2;

    const char* check_sql = "SELECT 1 FROM users WHERE username = ? LIMIT 1";
    if (mysql_stmt_prepare(stmt, check_sql, strlen(check_sql))) {
        mysql_stmt_close(stmt);
        return -2;
    }

    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char*)username.c_str();
    bind[0].buffer_length = username.length();

    if (mysql_stmt_bind_param(stmt, bind)) {
        mysql_stmt_close(stmt);
        return -2;
    }
    if (mysql_stmt_execute(stmt)) {
        mysql_stmt_close(stmt);
        return -2;
    }
    mysql_stmt_store_result(stmt);
    int cnt = mysql_stmt_num_rows(stmt);
    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);

    if (cnt > 0) return -1; // 用户名重复

    // 插入新用户
    stmt = mysql_stmt_init(conn_);
    const char* insert_sql = "INSERT INTO users (username, password_hash, salt) VALUES (?, ?, ?)";
    if (mysql_stmt_prepare(stmt, insert_sql, strlen(insert_sql))) {
        mysql_stmt_close(stmt);
        return -2;
    }

    MYSQL_BIND ins_bind[3];
    memset(ins_bind, 0, sizeof(ins_bind));
    ins_bind[0].buffer_type = MYSQL_TYPE_STRING;
    ins_bind[0].buffer = (char*)username.c_str();
    ins_bind[0].buffer_length = username.length();

    ins_bind[1].buffer_type = MYSQL_TYPE_STRING;
    ins_bind[1].buffer = (char*)password_hash.c_str();
    ins_bind[1].buffer_length = password_hash.length();

    ins_bind[2].buffer_type = MYSQL_TYPE_STRING;
    ins_bind[2].buffer = (char*)salt.c_str();
    ins_bind[2].buffer_length = salt.length();

    if (mysql_stmt_bind_param(stmt, ins_bind) || mysql_stmt_execute(stmt)) {
        mysql_stmt_close(stmt);
        return -2;
    }

    mysql_stmt_close(stmt);
    return 0;
}

bool Database::GetUserInfo(const std::string& username, std::string& out_salt, std::string& out_hash, int& out_userid) {
    MYSQL_STMT* stmt = mysql_stmt_init(conn_);
    if (!stmt) return false;

    const char* sql = "SELECT id, password_hash, salt FROM users WHERE username = ?";
    if (mysql_stmt_prepare(stmt, sql, strlen(sql))) {
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND param[1];
    memset(param, 0, sizeof(param));
    param[0].buffer_type = MYSQL_TYPE_STRING;
    param[0].buffer = (char*)username.c_str();
    param[0].buffer_length = username.length();
    if (mysql_stmt_bind_param(stmt, param)) {
        mysql_stmt_close(stmt);
        return false;
    }
    if (mysql_stmt_execute(stmt)) {
        mysql_stmt_close(stmt);
        return false;
    }

    int userid = 0;
    char hash[65] = {0};
    char salt[33] = {0};
    unsigned long hash_len, salt_len;
    MYSQL_BIND res[3];
    memset(res, 0, sizeof(res));
    res[0].buffer_type = MYSQL_TYPE_LONG;
    res[0].buffer = &userid;
    res[1].buffer_type = MYSQL_TYPE_STRING;
    res[1].buffer = hash;
    res[1].buffer_length = sizeof(hash) - 1;
    res[1].length = &hash_len;
    res[2].buffer_type = MYSQL_TYPE_STRING;
    res[2].buffer = salt;
    res[2].buffer_length = sizeof(salt) - 1;
    res[2].length = &salt_len;

    if (mysql_stmt_bind_result(stmt, res)) {
        mysql_stmt_close(stmt);
        return false;
    }
    mysql_stmt_store_result(stmt);
    if (mysql_stmt_num_rows(stmt) == 0) {
        mysql_stmt_free_result(stmt);
        mysql_stmt_close(stmt);
        return false;
    }
    if (mysql_stmt_fetch(stmt) != 0) {
        mysql_stmt_free_result(stmt);
        mysql_stmt_close(stmt);
        return false;
    }
    hash[hash_len] = '\0';
    salt[salt_len] = '\0';

    out_salt = salt;
    out_hash = hash;
    out_userid = userid;

    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);
    return true;
}
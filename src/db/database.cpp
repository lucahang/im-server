#include "db/database.h"
#include <stdexcept>
#include <cstring>

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

    if (cnt > 0) return -1;

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

void Database::CheckError(int ret, MYSQL_STMT* stmt) {
    if (ret != 0) {
        std::string err = stmt ? mysql_stmt_error(stmt) : mysql_error(conn_);
        throw DBException(err);
    }
}

void Database::SaveMessage(const im::HistoryMessage& msg,
                                    std::string session_id,
                                    int32_t msg_type) {
    std::lock_guard<std::mutex> lock(mutex_);
    MYSQL_STMT* stmt = mysql_stmt_init(conn_);
    if (!stmt) throw DBException("mysql_stmt_init failed");

    const char* sql =
        "INSERT INTO im_messages "
        "(msg_id, session_id, sender_id, receiver_id, group_id, content, msg_type, status, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

    int ret = mysql_stmt_prepare(stmt, sql, strlen(sql));
    if (ret != 0) {
        mysql_stmt_close(stmt);
        CheckError(ret, stmt);
    }

    // 准备绑定参数 —— 必须确保在 execute 期间字符串指针有效
    const std::string& sender   = msg.sender();
    const std::string& receiver = msg.receiver();
    const std::string& group    = msg.group_id();
    const std::string& content  = msg.content();

    int64_t msg_id    = msg.msg_id();
    int64_t timestamp = msg.timestamp();
    int32_t status    = 0;   // 默认正常
    int32_t type      = msg_type;

    MYSQL_BIND bind[9];
    memset(bind, 0, sizeof(bind));

    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].buffer = &msg_id;

    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (char*)session_id.c_str();
    bind[1].buffer_length = session_id.length();

    bind[2].buffer_type = MYSQL_TYPE_STRING;
    bind[2].buffer = (char*)sender.c_str();
    bind[2].buffer_length = sender.length();

    bind[3].buffer_type = MYSQL_TYPE_STRING;
    bind[3].buffer = (char*)receiver.c_str();
    bind[3].buffer_length = receiver.length();

    bind[4].buffer_type = MYSQL_TYPE_STRING;
    bind[4].buffer = (char*)group.c_str();
    bind[4].buffer_length = group.length();

    bind[5].buffer_type = MYSQL_TYPE_STRING;
    bind[5].buffer = (char*)content.c_str();
    bind[5].buffer_length = content.length();

    bind[6].buffer_type = MYSQL_TYPE_TINY;
    bind[6].buffer = &type;

    bind[7].buffer_type = MYSQL_TYPE_TINY;
    bind[7].buffer = &status;

    bind[8].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[8].buffer = &timestamp;

    ret = mysql_stmt_bind_param(stmt, bind);
    if (ret != 0) {
        mysql_stmt_close(stmt);
        CheckError(ret, stmt);
    }

    ret = mysql_stmt_execute(stmt);
    if (ret != 0) {
        mysql_stmt_close(stmt);
        CheckError(ret, stmt);
    }

    mysql_stmt_close(stmt);
}

void Database::UpsertUserSession(const std::string& user_id,
                                          const std::string& session_id,
                                          int32_t unread_count,
                                          const std::string& last_msg,
                                          int64_t updated_at) {
    std::lock_guard<std::mutex> lock(mutex_);

    MYSQL_STMT* stmt = mysql_stmt_init(conn_);
    if (!stmt) throw DBException("mysql_stmt_init failed");

    const char* sql =
        "INSERT INTO im_user_sessions (user_id, session_id, unread_count, last_msg, updated_at) "
        "VALUES (?, ?, ?, ?, ?) "
        "ON DUPLICATE KEY UPDATE unread_count = VALUES(unread_count), "
        "last_msg = VALUES(last_msg), updated_at = VALUES(updated_at)";

    if (mysql_stmt_prepare(stmt, sql, strlen(sql))) {
        mysql_stmt_close(stmt);
        CheckError(1, stmt);
    }

    MYSQL_BIND bind[5];
    memset(bind, 0, sizeof(bind));

    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char*)user_id.c_str();
    bind[0].buffer_length = user_id.length();

    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (char*)session_id.c_str();
    bind[1].buffer_length = session_id.length();

    bind[2].buffer_type = MYSQL_TYPE_LONG;
    bind[2].buffer = &unread_count;

    bind[3].buffer_type = MYSQL_TYPE_STRING;
    bind[3].buffer = (char*)last_msg.c_str();
    bind[3].buffer_length = last_msg.length();

    bind[4].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[4].buffer = &updated_at;

    if (mysql_stmt_bind_param(stmt, bind)) {
        mysql_stmt_close(stmt);
        CheckError(1, stmt);
    }
    if (mysql_stmt_execute(stmt)) {
        mysql_stmt_close(stmt);
        CheckError(1, stmt);
    }

    mysql_stmt_close(stmt);
}

std::vector<im::HistoryMessage> Database::GetMessagesBySession(
        const std::string& session_id,
        int64_t start,
        int32_t count) {
    std::lock_guard<std::mutex> lock(mutex_);

    MYSQL_STMT* stmt = mysql_stmt_init(conn_);
    if (!stmt) throw DBException("mysql_stmt_init failed");

    // 按 created_at 降序排序，支持分页
    const char* sql =
        "SELECT msg_id, sender_id, receiver_id, group_id, content, created_at "
        "FROM im_messages "
        "WHERE session_id = ? "
        "ORDER BY created_at DESC "
        "LIMIT ?, ?";

    if (mysql_stmt_prepare(stmt, sql, strlen(sql))) {
        std::string err = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        throw DBException("Prepare failed: " + err);
    }

    // 绑定参数：session_id, start, count
    MYSQL_BIND bind[3];
    memset(bind, 0, sizeof(bind));

    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char*)session_id.c_str();
    bind[0].buffer_length = session_id.length();

    bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[1].buffer = &start;

    bind[2].buffer_type = MYSQL_TYPE_LONG;
    bind[2].buffer = &count;

    if (mysql_stmt_bind_param(stmt, bind)) {
        std::string err = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        throw DBException("Bind param failed: " + err);
    }

    if (mysql_stmt_execute(stmt)) {
        std::string err = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        throw DBException("Execute failed: " + err);
    }

    // 绑定结果列
    int64_t msg_id;
    char sender[65] = {0};
    char receiver[65] = {0};
    char group[65] = {0};
    char content[2048] = {0};      // 假设单条消息不超过2048字节，可根据实际情况调整或动态分配
    int64_t created_at;
    unsigned long sender_len, receiver_len, group_len, content_len;

    MYSQL_BIND result[6];
    memset(result, 0, sizeof(result));

    result[0].buffer_type = MYSQL_TYPE_LONGLONG;
    result[0].buffer = &msg_id;

    result[1].buffer_type = MYSQL_TYPE_STRING;
    result[1].buffer = sender;
    result[1].buffer_length = sizeof(sender) - 1;
    result[1].length = &sender_len;

    result[2].buffer_type = MYSQL_TYPE_STRING;
    result[2].buffer = receiver;
    result[2].buffer_length = sizeof(receiver) - 1;
    result[2].length = &receiver_len;

    result[3].buffer_type = MYSQL_TYPE_STRING;
    result[3].buffer = group;
    result[3].buffer_length = sizeof(group) - 1;
    result[3].length = &group_len;

    result[4].buffer_type = MYSQL_TYPE_STRING;
    result[4].buffer = content;
    result[4].buffer_length = sizeof(content) - 1;
    result[4].length = &content_len;

    result[5].buffer_type = MYSQL_TYPE_LONGLONG;
    result[5].buffer = &created_at;

    if (mysql_stmt_bind_result(stmt, result)) {
        std::string err = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        throw DBException("Bind result failed: " + err);
    }

    // 存储结果集
    if (mysql_stmt_store_result(stmt)) {
        std::string err = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        throw DBException("Store result failed: " + err);
    }

    std::vector<im::HistoryMessage> messages;
    while (true) {
        int ret = mysql_stmt_fetch(stmt);
        if (ret == MYSQL_NO_DATA) break;
        if (ret == 1) { // 错误
            std::string err = mysql_stmt_error(stmt);
            mysql_stmt_free_result(stmt);
            mysql_stmt_close(stmt);
            throw DBException("Fetch failed: " + err);
        }

        im::HistoryMessage msg;
        msg.set_msg_id(msg_id);
        msg.set_sender(std::string(sender, sender_len));
        msg.set_receiver(std::string(receiver, receiver_len));
        msg.set_group_id(std::string(group, group_len));
        msg.set_content(std::string(content, content_len));
        msg.set_timestamp(created_at);
        messages.push_back(std::move(msg));
    }

    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);
    return messages;
}

std::vector<im::Contact> Database::GetUserContacts(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    MYSQL_STMT* stmt = mysql_stmt_init(conn_);
    if (!stmt) throw DBException("mysql_stmt_init failed");

    // 自动释放语句资源的 RAII 包装
    struct StmtGuard {
        MYSQL_STMT* s;
        ~StmtGuard() { if (s) mysql_stmt_close(s); }
    } guard{stmt};

    // 1. 准备 SQL 语句（查询状态正常 status = 1 的好友）
    std::string sql = "SELECT friend_id, COALESCE(alias, '') FROM friendship WHERE user_id = ? AND status = 1";
    if (mysql_stmt_prepare(stmt, sql.c_str(), sql.length()) != 0) {
        throw DBException(std::string("mysql_stmt_prepare failed: ") + mysql_stmt_error(stmt));
    }

    // 2. 绑定输入参数 (user_id)
    // 假设 user_id 传入的是可转换为数字的字符串，绑定为 LONGLONG (bigint)
    long long target_user_id = 0;
    try {
        target_user_id = std::stoll(user_id);
    } catch (...) {
        throw DBException("Invalid user_id format");
    }

    MYSQL_BIND bind_param[1];
    memset(bind_param, 0, sizeof(bind_param));

    bind_param[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind_param[0].buffer = (void*)&target_user_id;
    bind_param[0].is_unsigned = 0;

    if (mysql_stmt_bind_param(stmt, bind_param) != 0) {
        throw DBException(std::string("mysql_stmt_bind_param failed: ") + mysql_stmt_error(stmt));
    }

    // 3. 执行查询
    if (mysql_stmt_execute(stmt) != 0) {
        throw DBException(std::string("mysql_stmt_execute failed: ") + mysql_stmt_error(stmt));
    }

    // 4. 绑定输出结果缓存
    long long friend_id = 0;
    char alias_buf[51] = {0};
    unsigned long alias_len = 0;
    bool is_null[2] = {false, false};

    MYSQL_BIND bind_result[2];
    memset(bind_result, 0, sizeof(bind_result));

    // friend_id (bigint)
    bind_result[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind_result[0].buffer = (void*)&friend_id;
    bind_result[0].is_null = &is_null[0];

    // alias (varchar(50))
    bind_result[1].buffer_type = MYSQL_TYPE_STRING;
    bind_result[1].buffer = (void*)alias_buf;
    bind_result[1].buffer_length = sizeof(alias_buf);
    bind_result[1].length = &alias_len;
    bind_result[1].is_null = &is_null[1];

    if (mysql_stmt_bind_result(stmt, bind_result) != 0) {
        throw DBException(std::string("mysql_stmt_bind_result failed: ") + mysql_stmt_error(stmt));
    }

    // 缓冲全部结果集中在本地
    if (mysql_stmt_store_result(stmt) != 0) {
        throw DBException(std::string("mysql_stmt_store_result failed: ") + mysql_stmt_error(stmt));
    }

    // 5. 循环读取结果
    std::vector<im::Contact> contacts;
    while (mysql_stmt_fetch(stmt) == 0) {
        im::Contact contact;
        contact.set_user_id(std::to_string(friend_id));
        
        if (!is_null[1] && alias_len > 0) {
            contact.set_alias(std::string(alias_buf, alias_len));
        } else {
            contact.set_alias("");
        }

        contacts.push_back(std::move(contact));
        memset(alias_buf, 0, sizeof(alias_buf)); // 清空 buffer 供下次使用
    }

    return contacts;
}
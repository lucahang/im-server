#pragma once
#include <string>
#include <mysql/mysql.h>
#include "message.pb.h" 

class Database {
public:
    Database(const std::string& host, const std::string& user,
             const std::string& pass, const std::string& db, int port = 3306);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    int RegisterUser(const std::string& username, const std::string& password_hash, const std::string& salt);
    bool GetUserInfo(const std::string& username, std::string& out_salt, std::string& out_hash, int& out_userid);

    /**
     * 保存一条历史消息到 im_messages 表
     * @param msg        消息对象（protobuf 定义）
     * @param session_id 会话ID（单聊：single:uid1:uid2，群聊：group:gid）
     * @param msg_type   消息类型（0-文本）
     */
    void SaveMessage(const im::HistoryMessage& msg,
                     std::string session_id,
                     int32_t msg_type = 0);

    /**
     * 插入或更新用户会话记录（UPSERT）
     */
    void UpsertUserSession(const std::string& user_id,
                           const std::string& session_id,
                           int32_t unread_count,
                           const std::string& last_msg,
                           int64_t updated_at);

    /**
     * 从 im_messages 表按会话ID分页查询历史消息（降序：最新消息在前）
     * @param session_id 会话ID
     * @param start      偏移量（0 表示最新一条）
     * @param count      期望拉取条数
     * @return           消息列表（按时间降序）
     */
    std::vector<im::HistoryMessage> GetMessagesBySession(const std::string& session_id,
                                                        int64_t start,
                                                        int32_t count);  
                                                        
    std::vector<im::Contact> GetUserContacts(const std::string& user_id);
private:
    void CheckError(int ret, MYSQL_STMT* stmt = nullptr);
    MYSQL* conn_;
    std::mutex mutex_;
};

// 数据库异常
class DBException : public std::runtime_error {
public:
    explicit DBException(const std::string& msg) : std::runtime_error(msg) {}
};
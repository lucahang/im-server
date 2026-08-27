#pragma once
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <string>

class Connection;

class UserManager {
public:
    using ConnPtr = std::shared_ptr<Connection>;

    void AddUser(const std::string& userId, ConnPtr conn);
    void RemoveUser(const std::string& userId);
    ConnPtr GetUser(const std::string& userId);
    bool FindUser(const std::string& userId);
    
private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::weak_ptr<Connection>> users_;
};
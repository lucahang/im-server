#include "user_manager.h"
#include "connection.h"

void UserManager::AddUser(const std::string& userId, ConnPtr conn) {
    std::unique_lock lock(mutex_);
    users_[userId] = conn;
}

void UserManager::RemoveUser(const std::string& userId) {
    std::unique_lock lock(mutex_);
    users_.erase(userId);
}

UserManager::ConnPtr UserManager::GetUser(const std::string& userId) {
    std::shared_lock lock(mutex_);
    auto it = users_.find(userId);
    if (it != users_.end()) {
        return it->second.lock();
    }
    return nullptr;
}
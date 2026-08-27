#include "business/user_manager.h"
#include "net/connection.h"

void UserManager::AddUser(const std::string& userId, ConnPtr conn) {
    std::unique_lock lock(mutex_);
    users_[userId] = conn;
}

bool UserManager::FindUser(const std::string& userId){
    if(users_.find(userId)==users_.end()){
        return false;
    }
    return true;
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
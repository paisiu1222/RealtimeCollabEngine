#include "network/SessionManager.h"
#include <algorithm>

namespace network {

// ==================== 单例实现 ====================

SessionManager& SessionManager::getInstance() {
    static SessionManager instance;
    return instance;
}

SessionManager::SessionManager() : logger(utils::Logger::getInstance()) {
    logger.info("SessionManager initialized");
}

// ==================== 房间管理 ====================

std::shared_ptr<Room> SessionManager::getOrCreateRoom(
    const std::string& roomId, 
    const std::string& docId
) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = rooms_.find(roomId);
    if (it != rooms_.end()) {
        return it->second;
    }
    
    // 创建新房间
    auto room = std::make_shared<Room>(roomId, docId);
    rooms_[roomId] = room;
    
    logger.info("Created new room: " + roomId + " for document: " + docId);
    
    return room;
}

std::shared_ptr<Room> SessionManager::getRoom(const std::string& roomId) const {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = rooms_.find(roomId);
    if (it != rooms_.end()) {
        return it->second;
    }
    
    return nullptr;
}

bool SessionManager::removeRoom(const std::string& roomId) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        return false;
    }
    
    // 清理用户映射
    auto room = it->second;
    auto users = room->getUsers();
    for (const auto& userId : users) {
        userRooms_.erase(userId);
    }
    
    rooms_.erase(it);
    logger.info("Removed room: " + roomId);
    
    return true;
}

// ==================== 用户管理 ====================

bool SessionManager::addUserToRoom(
    const std::string& roomId, 
    const std::string& userId, 
    const std::string& connectionId
) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    // 获取或创建房间
    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        logger.error("Room not found: " + roomId);
        return false;
    }
    
    auto room = it->second;
    
    // 如果用户已在其他房间，先离开
    auto userIt = userRooms_.find(userId);
    if (userIt != userRooms_.end() && userIt->second != roomId) {
        std::string oldRoomId = userIt->second;
        auto oldRoomIt = rooms_.find(oldRoomId);
        if (oldRoomIt != rooms_.end()) {
            oldRoomIt->second->removeUser(userId);
        }
    }
    
    // 添加用户到房间
    bool success = room->addUser(userId, connectionId);
    if (success) {
        userRooms_[userId] = roomId;
    }
    
    return success;
}

bool SessionManager::removeUserFromRoom(
    const std::string& roomId, 
    const std::string& userId
) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        return false;
    }
    
    auto room = it->second;
    bool success = room->removeUser(userId);
    
    if (success) {
        userRooms_.erase(userId);
        
        // 如果房间为空，可以选择删除
        if (room->getUserCount() == 0) {
            logger.info("Room " + roomId + " is now empty");
            // 可选：自动删除空房间
            // rooms_.erase(it);
        }
    }
    
    return success;
}

std::string SessionManager::getUserRoom(const std::string& userId) const {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = userRooms_.find(userId);
    if (it != userRooms_.end()) {
        return it->second;
    }
    
    return "";
}

size_t SessionManager::getActiveRoomCount() const {
    std::lock_guard<std::mutex> lock(managerMutex);
    return rooms_.size();
}

std::vector<std::string> SessionManager::getAllRoomIds() const {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    std::vector<std::string> roomIds;
    for (const auto& pair : rooms_) {
        roomIds.push_back(pair.first);
    }
    
    return roomIds;
}

void SessionManager::cleanupEmptyRooms() {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    std::vector<std::string> emptyRooms;
    
    for (const auto& pair : rooms_) {
        if (pair.second->getUserCount() == 0) {
            emptyRooms.push_back(pair.first);
        }
    }
    
    for (const auto& roomId : emptyRooms) {
        logger.info("Cleaning up empty room: " + roomId);
        
        rooms_.erase(roomId);
    }
    
    if (!emptyRooms.empty()) {
        logger.info("Cleaned up " + std::to_string(emptyRooms.size()) + " empty rooms");
    }
}

} // namespace network

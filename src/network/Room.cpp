#include "network/Room.h"
#include <algorithm>

namespace network {

// ==================== 构造函数 ====================

Room::Room(const std::string& roomId, const std::string& docId)
    : roomId(roomId), docId(docId), document(docId), logger(utils::Logger::getInstance()) {
    logger.info("Room created: " + roomId + ", doc: " + docId);
}

// ==================== 用户管理 ====================

bool Room::addUser(const std::string& userId, const std::string& connectionId) {
    std::lock_guard<std::mutex> lock(roomMutex);
    
    if (users_.find(userId) != users_.end()) {
        logger.warning("User " + userId + " already in room " + roomId);
        return false;
    }
    
    users_[userId] = connectionId;
    logger.info("User " + userId + " joined room " + roomId + 
               " (total: " + std::to_string(users_.size()) + ")");
    
    return true;
}

bool Room::removeUser(const std::string& userId) {
    std::lock_guard<std::mutex> lock(roomMutex);
    
    auto it = users_.find(userId);
    if (it == users_.end()) {
        logger.warning("User " + userId + " not in room " + roomId);
        return false;
    }
    
    users_.erase(it);
    logger.info("User " + userId + " left room " + roomId + 
               " (remaining: " + std::to_string(users_.size()) + ")");
    
    return true;
}

size_t Room::getUserCount() const {
    std::lock_guard<std::mutex> lock(roomMutex);
    return users_.size();
}

bool Room::hasUser(const std::string& userId) const {
    std::lock_guard<std::mutex> lock(roomMutex);
    return users_.find(userId) != users_.end();
}

std::set<std::string> Room::getUsers() const {
    std::lock_guard<std::mutex> lock(roomMutex);
    std::set<std::string> userIds;
    for (const auto& pair : users_) {
        userIds.insert(pair.first);
    }
    return userIds;
}

std::string Room::getUserConnection(const std::string& userId) const {
    std::lock_guard<std::mutex> lock(roomMutex);
    auto it = users_.find(userId);
    if (it != users_.end()) {
        return it->second;
    }
    return "";
}

// ==================== 操作应用与广播 ====================

core::OperationResult Room::applyAndBroadcast(
    const core::Operation& op, 
    const std::string& senderId,
    const BroadcastCallback& /*broadcastCallback*/
) {
    // 应用操作到文档
    auto result = document.applyOperation(op);
    
    if (result != core::OperationResult::SUCCESS) {
        logger.error("Failed to apply operation in room " + roomId + 
                    ": result=" + std::to_string(static_cast<int>(result)));
        return result;
    }
    
    // 创建操作确认消息（发送给发送者）
    Message ackMsg;
    ackMsg.type = MessageType::OPERATION_ACK;
    ackMsg.messageId = "ack_" + op.opId;
    ackMsg.timestamp = Message::getCurrentTimestamp();
    ackMsg.payload = {
        {"opId", op.opId},
        {"version", document.getVersion()},
        {"status", "success"}
    };
    
    // 广播操作给其他用户
    Message broadcastMsg;
    broadcastMsg.type = MessageType::OPERATION;
    broadcastMsg.messageId = "broadcast_" + op.opId;
    broadcastMsg.timestamp = Message::getCurrentTimestamp();
    broadcastMsg.payload = {
        {"opId", op.opId},
        {"userId", op.userId},
        {"version", op.version},
        {"type", core::operationTypeToString(op.type)},
        {"position", op.position},
        {"content", op.content}
    };
    
    // 发送ACK给发送者
    if (broadcastCallback_) {
        broadcastCallback_(senderId, ackMsg.toJson());
    }
    
    // 广播操作给其他人
    broadcastToOthers(senderId, broadcastMsg.toJson());
    
    logger.debug("Operation applied and broadcasted in room " + roomId + 
                ": version=" + std::to_string(document.getVersion()));
    
    return result;
}

core::DocumentState& Room::getDocument() {
    return document;
}

const core::DocumentState& Room::getDocument() const {
    return document;
}

uint64_t Room::getVersion() const {
    return document.getVersion();
}

// ==================== 内部方法 ====================

void Room::broadcastToOthers(
    const std::string& senderId, 
    const std::string& message
) {
    std::lock_guard<std::mutex> lock(roomMutex);
    
    if (!broadcastCallback_) {
        logger.warning("No broadcast callback set for room " + roomId);
        return;
    }
    
    int count = 0;
    for (const auto& pair : users_) {
        if (pair.first != senderId) {
            broadcastCallback_(pair.first, message);
            count++;
        }
    }
    
    logger.debug("Broadcasted message to " + std::to_string(count) + 
                " users in room " + roomId);
}

void Room::broadcastToAll(const std::string& message) {
    std::lock_guard<std::mutex> lock(roomMutex);
    
    if (!broadcastCallback_) {
        logger.warning("No broadcast callback set for room " + roomId);
        return;
    }
    
    for (const auto& pair : users_) {
        broadcastCallback_(pair.first, message);
    }
}

} // namespace network

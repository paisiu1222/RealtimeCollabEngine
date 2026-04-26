#include "network/MessageProtocol.h"
#include <uuid/uuid.h>
#include <sstream>
#include <iomanip>

namespace network {

std::string MessageValidator::lastError = "";

// ==================== 辅助函数 ====================

static std::string generateMessageId() {
    uuid_t uuid;
    uuid_generate(uuid);
    char uuid_str[37];
    uuid_unparse_lower(uuid, uuid_str);
    return std::string(uuid_str);
}

static std::string messageTypeToString(MessageType type) {
    switch (type) {
        case MessageType::CONNECT: return "connect";
        case MessageType::DISCONNECT: return "disconnect";
        case MessageType::HEARTBEAT: return "heartbeat";
        case MessageType::JOIN_ROOM: return "join_room";
        case MessageType::LEAVE_ROOM: return "leave_room";
        case MessageType::ROOM_INFO: return "room_info";
        case MessageType::OPERATION: return "operation";
        case MessageType::OPERATION_ACK: return "operation_ack";
        case MessageType::DOCUMENT_SYNC: return "document_sync";
        case MessageType::USER_JOIN: return "user_join";
        case MessageType::USER_LEAVE: return "user_leave";
        case MessageType::CURSOR_UPDATE: return "cursor_update";
        case MessageType::ERROR: return "error";
        default: return "unknown";
    }
}

static MessageType stringToMessageType(const std::string& str) {
    if (str == "connect") return MessageType::CONNECT;
    if (str == "disconnect") return MessageType::DISCONNECT;
    if (str == "heartbeat") return MessageType::HEARTBEAT;
    if (str == "join_room") return MessageType::JOIN_ROOM;
    if (str == "leave_room") return MessageType::LEAVE_ROOM;
    if (str == "room_info") return MessageType::ROOM_INFO;
    if (str == "operation") return MessageType::OPERATION;
    if (str == "operation_ack") return MessageType::OPERATION_ACK;
    if (str == "document_sync") return MessageType::DOCUMENT_SYNC;
    if (str == "user_join") return MessageType::USER_JOIN;
    if (str == "user_leave") return MessageType::USER_LEAVE;
    if (str == "cursor_update") return MessageType::CURSOR_UPDATE;
    if (str == "error") return MessageType::ERROR;
    throw std::runtime_error("Unknown message type: " + str);
}

// ==================== Message 实现 ====================

std::string Message::toJson() const {
    nlohmann::json json;
    json["type"] = messageTypeToString(type);
    json["messageId"] = messageId;
    json["timestamp"] = timestamp;
    json["payload"] = payload;
    return json.dump();
}

Message Message::fromJson(const std::string& json) {
    Message msg;
    try {
        auto j = nlohmann::json::parse(json);
        
        if (!MessageValidator::validateBasicStructure(j)) {
            throw std::runtime_error(MessageValidator::getLastError());
        }
        
        msg.type = stringToMessageType(j["type"]);
        msg.messageId = j["messageId"];
        msg.timestamp = j["timestamp"];
        msg.payload = j["payload"];
        
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to parse message: ") + e.what());
    }
    
    return msg;
}

std::string Message::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    
    return ss.str();
}

// ==================== MessageFactory 实现 ====================

Message MessageFactory::createConnectMessage(const std::string& userId, const std::string& token) {
    Message msg;
    msg.type = MessageType::CONNECT;
    msg.messageId = generateMessageId();
    msg.timestamp = Message::getCurrentTimestamp();
    msg.payload = {
        {"userId", userId},
        {"token", token}
    };
    return msg;
}

Message MessageFactory::createDisconnectMessage(const std::string& reason) {
    Message msg;
    msg.type = MessageType::DISCONNECT;
    msg.messageId = generateMessageId();
    msg.timestamp = Message::getCurrentTimestamp();
    msg.payload = {{"reason", reason}};
    return msg;
}

Message MessageFactory::createHeartbeatMessage() {
    Message msg;
    msg.type = MessageType::HEARTBEAT;
    msg.messageId = generateMessageId();
    msg.timestamp = Message::getCurrentTimestamp();
    msg.payload = {{"ping", true}};
    return msg;
}

Message MessageFactory::createHeartbeatResponse() {
    Message msg;
    msg.type = MessageType::HEARTBEAT;
    msg.messageId = generateMessageId();
    msg.timestamp = Message::getCurrentTimestamp();
    msg.payload = {{"pong", true}};
    return msg;
}

Message MessageFactory::createJoinRoomMessage(const std::string& roomId) {
    Message msg;
    msg.type = MessageType::JOIN_ROOM;
    msg.messageId = generateMessageId();
    msg.timestamp = Message::getCurrentTimestamp();
    msg.payload = {{"roomId", roomId}};
    return msg;
}

Message MessageFactory::createLeaveRoomMessage(const std::string& roomId) {
    Message msg;
    msg.type = MessageType::LEAVE_ROOM;
    msg.messageId = generateMessageId();
    msg.timestamp = Message::getCurrentTimestamp();
    msg.payload = {{"roomId", roomId}};
    return msg;
}

Message MessageFactory::createRoomInfoMessage(const std::string& roomId, int userCount) {
    Message msg;
    msg.type = MessageType::ROOM_INFO;
    msg.messageId = generateMessageId();
    msg.timestamp = Message::getCurrentTimestamp();
    msg.payload = {
        {"roomId", roomId},
        {"userCount", userCount}
    };
    return msg;
}

Message MessageFactory::createOperationMessage(
    const std::string& opId,
    const std::string& userId,
    uint64_t version,
    const std::string& opType,
    size_t position,
    const std::string& content
) {
    Message msg;
    msg.type = MessageType::OPERATION;
    msg.messageId = generateMessageId();
    msg.timestamp = Message::getCurrentTimestamp();
    msg.payload = {
        {"opId", opId},
        {"userId", userId},
        {"version", version},
        {"operation", {
            {"type", opType},
            {"position", position},
            {"content", content}
        }}
    };
    return msg;
}

Message MessageFactory::createOperationAckMessage(const std::string& opId, uint64_t version) {
    Message msg;
    msg.type = MessageType::OPERATION_ACK;
    msg.messageId = generateMessageId();
    msg.timestamp = Message::getCurrentTimestamp();
    msg.payload = {
        {"opId", opId},
        {"version", version}
    };
    return msg;
}

Message MessageFactory::createDocumentSyncMessage(const std::string& content, uint64_t version) {
    Message msg;
    msg.type = MessageType::DOCUMENT_SYNC;
    msg.messageId = generateMessageId();
    msg.timestamp = Message::getCurrentTimestamp();
    msg.payload = {
        {"content", content},
        {"version", version}
    };
    return msg;
}

Message MessageFactory::createUserJoinMessage(const std::string& userId, const std::string& username) {
    Message msg;
    msg.type = MessageType::USER_JOIN;
    msg.messageId = generateMessageId();
    msg.timestamp = Message::getCurrentTimestamp();
    msg.payload = {
        {"userId", userId},
        {"username", username}
    };
    return msg;
}

Message MessageFactory::createUserLeaveMessage(const std::string& userId) {
    Message msg;
    msg.type = MessageType::USER_LEAVE;
    msg.messageId = generateMessageId();
    msg.timestamp = Message::getCurrentTimestamp();
    msg.payload = {{"userId", userId}};
    return msg;
}

Message MessageFactory::createCursorUpdateMessage(
    const std::string& userId,
    size_t position,
    size_t selectionStart,
    size_t selectionEnd
) {
    Message msg;
    msg.type = MessageType::CURSOR_UPDATE;
    msg.messageId = generateMessageId();
    msg.timestamp = Message::getCurrentTimestamp();
    msg.payload = {
        {"userId", userId},
        {"position", position},
        {"selectionStart", selectionStart},
        {"selectionEnd", selectionEnd}
    };
    return msg;
}

Message MessageFactory::createErrorMessage(const std::string& code, const std::string& message) {
    Message msg;
    msg.type = MessageType::ERROR;
    msg.messageId = generateMessageId();
    msg.timestamp = Message::getCurrentTimestamp();
    msg.payload = {
        {"code", code},
        {"message", message}
    };
    return msg;
}

// ==================== MessageValidator 实现 ====================

bool MessageValidator::validateBasicStructure(const nlohmann::json& json) {
    // 检查必需字段
    if (!json.contains("type")) {
        lastError = "Missing required field: type";
        return false;
    }
    
    if (!json.contains("messageId")) {
        lastError = "Missing required field: messageId";
        return false;
    }
    
    if (!json.contains("timestamp")) {
        lastError = "Missing required field: timestamp";
        return false;
    }
    
    if (!json.contains("payload")) {
        lastError = "Missing required field: payload";
        return false;
    }
    
    // 验证类型字段
    try {
        stringToMessageType(json["type"]);
    } catch (const std::exception&) {
        lastError = "Invalid message type: " + json["type"].get<std::string>();
        return false;
    }
    
    return true;
}

bool MessageValidator::validatePayload(MessageType type, const nlohmann::json& payload) {
    switch (type) {
        case MessageType::CONNECT:
            if (!payload.contains("userId") || !payload.contains("token")) {
                lastError = "CONNECT message requires userId and token";
                return false;
            }
            break;
            
        case MessageType::JOIN_ROOM:
        case MessageType::LEAVE_ROOM:
            if (!payload.contains("roomId")) {
                lastError = "Room message requires roomId";
                return false;
            }
            break;
            
        case MessageType::OPERATION:
            if (!payload.contains("opId") || !payload.contains("userId") || 
                !payload.contains("version") || !payload.contains("operation")) {
                lastError = "OPERATION message requires opId, userId, version, and operation";
                return false;
            }
            break;
            
        default:
            // 其他类型暂不严格验证
            break;
    }
    
    return true;
}

std::string MessageValidator::getLastError() {
    return lastError;
}

} // namespace network

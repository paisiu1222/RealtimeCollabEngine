#ifndef MESSAGE_PROTOCOL_H
#define MESSAGE_PROTOCOL_H

#include <string>
#include <nlohmann/json.hpp>
#include <chrono>

namespace network {

/**
 * 消息类型枚举
 */
enum class MessageType {
    // 连接管理
    CONNECT,        // 客户端连接
    DISCONNECT,     // 客户端断开
    HEARTBEAT,      // 心跳请求/响应
    
    // 房间管理
    JOIN_ROOM,      // 加入房间
    LEAVE_ROOM,     // 离开房间
    ROOM_INFO,      // 房间信息
    
    // 文档操作
    OPERATION,      // 编辑操作
    OPERATION_ACK,  // 操作确认
    DOCUMENT_SYNC,  // 文档同步
    
    // 用户状态
    USER_JOIN,      // 用户加入
    USER_LEAVE,     // 用户离开
    CURSOR_UPDATE,  // 光标位置更新
    
    // 错误处理
    ERROR,          // 错误消息
};

/**
 * 消息结构体
 */
struct Message {
    MessageType type;
    std::string messageId;      // 消息唯一ID
    std::string timestamp;      // ISO8601时间戳
    nlohmann::json payload;     // 消息负载
    
    /**
     * 将消息序列化为JSON字符串
     */
    std::string toJson() const;
    
    /**
     * 从JSON字符串解析消息
     */
    static Message fromJson(const std::string& json);
    
    /**
     * 获取当前时间戳（ISO8601格式）
     */
    static std::string getCurrentTimestamp();
};

/**
 * 消息工厂类 - 便捷创建各种类型的消息
 */
class MessageFactory {
public:
    // 连接管理消息
    static Message createConnectMessage(const std::string& userId, const std::string& token);
    static Message createDisconnectMessage(const std::string& reason = "");
    static Message createHeartbeatMessage();
    static Message createHeartbeatResponse();
    
    // 房间管理消息
    static Message createJoinRoomMessage(const std::string& roomId);
    static Message createLeaveRoomMessage(const std::string& roomId);
    static Message createRoomInfoMessage(const std::string& roomId, int userCount);
    
    // 文档操作消息
    static Message createOperationMessage(
        const std::string& opId,
        const std::string& userId,
        uint64_t version,
        const std::string& opType,
        size_t position,
        const std::string& content
    );
    static Message createOperationAckMessage(const std::string& opId, uint64_t version);
    static Message createDocumentSyncMessage(const std::string& content, uint64_t version);
    
    // 用户状态消息
    static Message createUserJoinMessage(const std::string& userId, const std::string& username);
    static Message createUserLeaveMessage(const std::string& userId);
    static Message createCursorUpdateMessage(
        const std::string& userId,
        size_t position,
        size_t selectionStart = 0,
        size_t selectionEnd = 0
    );
    
    // 错误消息
    static Message createErrorMessage(const std::string& code, const std::string& message);
};

/**
 * 消息验证器
 */
class MessageValidator {
public:
    /**
     * 验证消息的基本结构
     */
    static bool validateBasicStructure(const nlohmann::json& json);
    
    /**
     * 验证特定类型消息的payload
     */
    static bool validatePayload(MessageType type, const nlohmann::json& payload);
    
    /**
     * 获取验证错误信息
     */
    static std::string getLastError();
    
private:
    static std::string lastError;
};

} // namespace network

#endif // MESSAGE_PROTOCOL_H

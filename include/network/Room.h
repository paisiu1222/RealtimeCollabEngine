#ifndef ROOM_H
#define ROOM_H

#include <string>
#include <set>
#include <map>
#include <mutex>
#include <functional>
#include "core/DocumentState.h"
#include "core/Operation.h"
#include "network/MessageProtocol.h"
#include "utils/Logger.h"

namespace network {

/**
 * 房间类
 * 管理一个协作会话房间，包含文档状态和所有连接的用户
 */
class Room {
public:
    /**
     * 消息广播回调函数类型
     */
    using BroadcastCallback = std::function<void(const std::string& userId, const std::string& message)>;
    
    /**
     * 构造函数
     * @param roomId 房间ID
     * @param docId 关联的文档ID
     */
    Room(const std::string& roomId, const std::string& docId);
    
    /**
     * 添加用户到房间
     * @param userId 用户ID
     * @param connectionId WebSocket连接ID
     * @return 是否成功
     */
    bool addUser(const std::string& userId, const std::string& connectionId);
    
    /**
     * 从房间移除用户
     * @param userId 用户ID
     * @return 是否成功
     */
    bool removeUser(const std::string& userId);
    
    /**
     * 获取房间内的用户数量
     */
    size_t getUserCount() const;
    
    /**
     * 检查用户是否在房间内
     */
    bool hasUser(const std::string& userId) const;
    
    /**
     * 获取所有用户ID列表
     */
    std::set<std::string> getUsers() const;
    
    /**
     * 获取用户的连接ID
     */
    std::string getUserConnection(const std::string& userId) const;
    
    /**
     * 应用操作并广播给其他用户
     * @param op 要应用的操作
     * @param senderId 发送者用户ID
     * @param broadcastCallback 广播回调函数
     * @return 操作结果
     */
    core::OperationResult applyAndBroadcast(
        const core::Operation& op, 
        const std::string& senderId,
        const BroadcastCallback& broadcastCallback
    );
    
    /**
     * 获取文档状态（用于初始化新加入的用户）
     */
    core::DocumentState& getDocument();
    const core::DocumentState& getDocument() const;
    
    /**
     * 获取房间ID
     */
    std::string getRoomId() const { return roomId; }
    
    /**
     * 获取文档ID
     */
    std::string getDocId() const { return docId; }
    
    /**
     * 获取当前版本号
     */
    uint64_t getVersion() const;
    
    /**
     * 设置广播回调（用于发送消息给用户）
     */
    void setBroadcastCallback(BroadcastCallback callback) { broadcastCallback_ = callback; }

private:
    /**
     * 向房间内除发送者外的所有用户广播消息
     */
    void broadcastToOthers(
        const std::string& senderId, 
        const std::string& message
    );
    
    /**
     * 向所有用户（包括发送者）广播消息
     */
    void broadcastToAll(const std::string& message);
    
    // ==================== 成员变量 ====================
    
    std::string roomId;                           // 房间ID
    std::string docId;                            // 关联文档ID
    core::DocumentState document;                 // 文档状态
    
    mutable std::mutex roomMutex;                 // 房间互斥锁
    std::map<std::string, std::string> users_;    // userId -> connectionId映射
    
    BroadcastCallback broadcastCallback_;         // 广播回调函数
    utils::Logger& logger;                        // 日志记录器
};

} // namespace network

#endif // ROOM_H

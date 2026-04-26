#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <crow.h>
#include <string>
#include <map>
#include <set>
#include <mutex>
#include <memory>
#include <functional>
#include <chrono>
#include "network/MessageProtocol.h"
#include "utils/Logger.h"

namespace network {

/**
 * WebSocket连接信息
 */
struct WebSocketConnection {
    std::string connectionId;         // 连接唯一ID
    std::string userId;               // 用户ID
    std::string username;             // 用户名
    std::string roomId;               // 当前所在房间
    std::chrono::system_clock::time_point lastHeartbeat; // 最后心跳时间
    bool isAuthenticated;             // 是否已认证
    crow::websocket::connection* wsConn; // Crow WebSocket连接指针
    
    WebSocketConnection() 
        : isAuthenticated(false), wsConn(nullptr) {
        lastHeartbeat = std::chrono::system_clock::now();
    }
};

/**
 * 房间信息
 */
struct RoomInfo {
    std::string roomId;
    std::string documentId;
    std::set<std::string> memberIds;  // 成员连接ID集合
    uint64_t currentVersion;          // 当前文档版本
    std::chrono::system_clock::time_point createdAt;
    
    RoomInfo() : currentVersion(0) {
        createdAt = std::chrono::system_clock::now();
    }
};

/**
 * WebSocket服务器类
 * 基于Crow框架实现
 */
class WebSocketServer {
public:
    /**
     * 构造函数
     * @param host 监听地址
     * @param port 监听端口
     */
    WebSocketServer(const std::string& host, int port);
    
    /**
     * 启动服务器
     */
    void start();
    
    /**
     * 停止服务器
     */
    void stop();
    
    /**
     * 获取服务器运行状态
     */
    bool isRunning() const { return running; }
    
    /**
     * 向指定房间广播消息
     * @param roomId 房间ID
     * @param message 消息内容（JSON字符串）
     * @param excludeConnId 排除的连接ID（可选）
     */
    void broadcastToRoom(const std::string& roomId, const std::string& message, 
                         const std::string& excludeConnId = "");
    
    /**
     * 向指定连接发送消息
     * @param connId 连接ID
     * @param message 消息内容
     */
    void sendToConnection(const std::string& connId, const std::string& message);
    
    /**
     * 获取房间信息
     */
    RoomInfo getRoomInfo(const std::string& roomId) const;
    
    /**
     * 获取在线用户数
     */
    size_t getOnlineUserCount() const;
    
    /**
     * 获取房间内的用户数
     */
    size_t getRoomUserCount(const std::string& roomId) const;
    
    /**
     * 设置消息处理回调
     */
    using MessageHandler = std::function<void(const std::string& connId, const Message& msg)>;
    void setMessageHandler(MessageHandler handler) { messageHandler = handler; }
    
private:
    // ==================== 内部方法 ====================
    
    /**
     * 初始化Crow应用和路由
     */
    void setupRoutes();
    
    /**
     * 处理WebSocket连接打开
     */
    void handleOpen(crow::websocket::connection& conn);
    
    /**
     * 处理WebSocket消息
     */
    void handleMessage(crow::websocket::connection& conn, const std::string& data, bool isBinary);
    
    /**
     * 处理WebSocket连接关闭
     */
    void handleClose(crow::websocket::connection& conn, const std::string& reason);
    
    /**
     * 处理错误
     */
    void handleError(crow::websocket::connection& conn, const std::string& error);
    
    // ==================== 业务逻辑方法 ====================
    
    /**
     * 处理连接请求
     */
    void handleConnect(const std::string& connId, const nlohmann::json& payload);
    
    /**
     * 处理断开连接
     */
    void handleDisconnect(const std::string& connId, const std::string& reason);
    
    /**
     * 处理心跳
     */
    void handleHeartbeat(const std::string& connId);
    
    /**
     * 处理加入房间
     */
    void handleJoinRoom(const std::string& connId, const nlohmann::json& payload);
    
    /**
     * 处理离开房间
     */
    void handleLeaveRoom(const std::string& connId, const nlohmann::json& payload);
    
    /**
     * 处理编辑操作
     */
    void handleOperation(const std::string& connId, const nlohmann::json& payload);
    
    /**
     * 处理光标更新
     */
    void handleCursorUpdate(const std::string& connId, const nlohmann::json& payload);
    
    // ==================== 连接管理方法 ====================
    
    /**
     * 添加连接
     */
    void addConnection(const std::string& connId, crow::websocket::connection* wsConn);
    
    /**
     * 移除连接
     */
    void removeConnection(const std::string& connId);
    
    /**
     * 获取连接信息
     */
    WebSocketConnection* getConnection(const std::string& connId);
    
    /**
     * 验证Token
     */
    bool validateToken(const std::string& userId, const std::string& token);
    
    /**
     * 生成连接ID
     */
    std::string generateConnectionId();
    
    // ==================== 心跳检测 ====================
    
    /**
     * 启动心跳检测线程
     */
    void startHeartbeatChecker();
    
    /**
     * 检查超时连接
     */
    void checkTimeoutConnections();
    
    // ==================== 成员变量 ====================
    
    std::string host;
    int port;
    bool running;
    
    crow::SimpleApp app;                          // Crow应用实例
    
    mutable std::mutex connectionsMutex;          // 连接表互斥锁
    std::map<std::string, WebSocketConnection> connections; // 连接ID -> 连接信息
    
    mutable std::mutex roomsMutex;                // 房间表互斥锁
    std::map<std::string, RoomInfo> rooms;        // 房间ID -> 房间信息
    
    MessageHandler messageHandler;                // 消息处理回调
    
    std::chrono::seconds heartbeatInterval;       // 心跳间隔
    std::chrono::seconds connectionTimeout;       // 连接超时时间
    
    utils::Logger& logger;                        // 日志记录器
};

} // namespace network

#endif // WEBSOCKET_SERVER_H

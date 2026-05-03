#include "network/WebSocketServer.h"
#include <uuid/uuid.h>
#include <thread>
#include <algorithm>

namespace network {

// ==================== 构造函数与析构函数 ====================

WebSocketServer::WebSocketServer(const std::string& host, int port)
    : host(host), port(port), running(false), 
      heartbeatInterval(30), connectionTimeout(90),
      logger(utils::Logger::getInstance()) {
    
    logger.info("WebSocketServer created on " + host + ":" + std::to_string(port));
}

// ==================== 启动与停止 ====================

void WebSocketServer::start() {
    if (running) {
        logger.warning("WebSocketServer is already running");
        return;
    }
    
    logger.info("Starting WebSocketServer...");
    
    // 设置路由
    setupRoutes();
    
    // 启动心跳检测线程
    startHeartbeatChecker();
    
    // 启动Crow服务器（非阻塞）
    running = true;
    
    // 配置Crow
    app.port(port);
    app.multithreaded();
    
    logger.info("WebSocketServer started on " + host + ":" + std::to_string(port));
    
    // 运行Crow应用（阻塞）
    app.run();
}

void WebSocketServer::stop() {
    if (!running) {
        logger.warning("WebSocketServer is not running");
        return;
    }
    
    logger.info("Stopping WebSocketServer...");
    running = false;
    
    // 关闭所有连接
    {
        std::lock_guard<std::mutex> lock(connectionsMutex);
        for (auto& pair : connections) {
            if (pair.second.wsConn) {
                try {
                    pair.second.wsConn->close("Server shutting down");
                } catch (...) {
                    // 忽略关闭错误
                }
            }
        }
        connections.clear();
    }
    
    {
        std::lock_guard<std::mutex> lock(roomsMutex);
        rooms.clear();
    }
    
    logger.info("WebSocketServer stopped");
}

// ==================== 消息广播与发送 ====================

void WebSocketServer::broadcastToRoom(const std::string& roomId, const std::string& message, 
                                      const std::string& excludeConnId) {
    std::lock_guard<std::mutex> roomLock(roomsMutex);
    
    auto it = rooms.find(roomId);
    if (it == rooms.end()) {
        logger.warning("Room not found: " + roomId);
        return;
    }
    
    const auto& memberIds = it->second.memberIds;
    
    std::lock_guard<std::mutex> connLock(connectionsMutex);
    
    for (const auto& connId : memberIds) {
        if (connId == excludeConnId) {
            continue; // 跳过排除的连接
        }
        
        auto connIt = connections.find(connId);
        if (connIt != connections.end() && connIt->second.wsConn) {
            try {
                connIt->second.wsConn->send_text(message);
            } catch (const std::exception& e) {
                logger.error("Failed to send message to " + connId + ": " + e.what());
            }
        }
    }
}

void WebSocketServer::sendToConnection(const std::string& connId, const std::string& message) {
    std::lock_guard<std::mutex> lock(connectionsMutex);
    
    auto it = connections.find(connId);
    if (it == connections.end()) {
        logger.warning("Connection not found: " + connId);
        return;
    }
    
    if (it->second.wsConn) {
        try {
            it->second.wsConn->send_text(message);
        } catch (const std::exception& e) {
            logger.error("Failed to send message to " + connId + ": " + e.what());
        }
    }
}

// ==================== 房间信息查询 ====================

RoomInfo WebSocketServer::getRoomInfo(const std::string& roomId) const {
    std::lock_guard<std::mutex> lock(roomsMutex);
    
    auto it = rooms.find(roomId);
    if (it != rooms.end()) {
        return it->second;
    }
    
    return RoomInfo(); // 返回空房间信息
}

size_t WebSocketServer::getOnlineUserCount() const {
    std::lock_guard<std::mutex> lock(connectionsMutex);
    return connections.size();
}

size_t WebSocketServer::getRoomUserCount(const std::string& roomId) const {
    std::lock_guard<std::mutex> lock(roomsMutex);
    
    auto it = rooms.find(roomId);
    if (it != rooms.end()) {
        return it->second.memberIds.size();
    }
    
    return 0;
}

// ==================== Crow路由设置 ====================

void WebSocketServer::setupRoutes() {
    // WebSocket端点
    CROW_WEBSOCKET_ROUTE(app, "/ws")
        .onopen([this](crow::websocket::connection& conn) {
            handleOpen(conn);
        })
        .onmessage([this](crow::websocket::connection& conn, const std::string& data, bool isBinary) {
            handleMessage(conn, data, isBinary);
        })
        .onclose([this](crow::websocket::connection& conn, const std::string& reason) {
            handleClose(conn, reason);
        })
        .onerror([this](crow::websocket::connection& conn, const std::string& error) {
            handleError(conn, error);
        });
    
    // HTTP健康检查端点
    CROW_ROUTE(app, "/health").methods(crow::HTTPMethod::GET)
    ([this]() {
        nlohmann::json response;
        response["status"] = "ok";
        response["onlineUsers"] = getOnlineUserCount();
        {
            std::lock_guard<std::mutex> lock(roomsMutex);
            response["activeRooms"] = rooms.size();
        }
        return crow::response(200, response.dump());
    });
    
    // HTTP API端点 - 获取房间列表
    CROW_ROUTE(app, "/api/rooms").methods(crow::HTTPMethod::GET)
    ([this]() {
        std::lock_guard<std::mutex> lock(roomsMutex);
        
        nlohmann::json response = nlohmann::json::array();
        for (const auto& pair : rooms) {
            nlohmann::json roomInfo;
            roomInfo["roomId"] = pair.first;
            roomInfo["documentId"] = pair.second.documentId;
            roomInfo["userCount"] = pair.second.memberIds.size();
            roomInfo["version"] = pair.second.currentVersion;
            response.push_back(roomInfo);
        }
        
        return crow::response(200, response.dump());
    });
    
    logger.info("Routes configured");
}

// ==================== WebSocket事件处理 ====================

void WebSocketServer::handleOpen(crow::websocket::connection& conn) {
    std::string connId = generateConnectionId();
    addConnection(connId, &conn);
    
    logger.info("New connection opened: " + connId);
    
    // 发送欢迎消息
    Message welcomeMsg = MessageFactory::createConnectMessage("", "");
    welcomeMsg.payload["status"] = "connected";
    welcomeMsg.payload["connectionId"] = connId;
    sendToConnection(connId, welcomeMsg.toJson());
}

void WebSocketServer::handleMessage(crow::websocket::connection& conn, const std::string& data, bool isBinary) {
    // 查找连接ID
    std::string connId;
    {
        std::lock_guard<std::mutex> lock(connectionsMutex);
        for (const auto& pair : connections) {
            if (pair.second.wsConn == &conn) {
                connId = pair.first;
                break;
            }
        }
    }
    
    if (connId.empty()) {
        logger.error("Received message from unknown connection");
        return;
    }
    
    try {
        // 解析消息
        Message msg = Message::fromJson(data);
        
        // 更新心跳时间
        {
            std::lock_guard<std::mutex> lock(connectionsMutex);
            auto it = connections.find(connId);
            if (it != connections.end()) {
                it->second.lastHeartbeat = std::chrono::system_clock::now();
            }
        }
        
        // 调用消息处理回调
        if (messageHandler) {
            messageHandler(connId, msg);
        }
        
        // 根据消息类型分发处理
        switch (msg.type) {
            case MessageType::CONNECT:
                handleConnect(connId, msg.payload);
                break;
            case MessageType::HEARTBEAT:
                handleHeartbeat(connId);
                break;
            case MessageType::JOIN_ROOM:
                handleJoinRoom(connId, msg.payload);
                break;
            case MessageType::LEAVE_ROOM:
                handleLeaveRoom(connId, msg.payload);
                break;
            case MessageType::OPERATION:
                handleOperation(connId, msg.payload);
                break;
            case MessageType::CURSOR_UPDATE:
                handleCursorUpdate(connId, msg.payload);
                break;
            default:
                logger.warning("Unhandled message type from " + connId);
                break;
        }
        
    } catch (const std::exception& e) {
        logger.error("Error processing message from " + connId + ": " + e.what());
        
        // 发送错误消息
        Message errorMsg = MessageFactory::createErrorMessage("PARSE_ERROR", e.what());
        sendToConnection(connId, errorMsg.toJson());
    }
}

void WebSocketServer::handleClose(crow::websocket::connection& conn, const std::string& reason) {
    // 查找并移除连接
    std::string connId;
    {
        std::lock_guard<std::mutex> lock(connectionsMutex);
        for (auto it = connections.begin(); it != connections.end(); ++it) {
            if (it->second.wsConn == &conn) {
                connId = it->first;
                break;
            }
        }
    }
    
    if (!connId.empty()) {
        logger.info("Connection closed: " + connId + ", reason: " + reason);
        handleDisconnect(connId, reason);
        removeConnection(connId);
    }
}

void WebSocketServer::handleError(crow::websocket::connection& conn, const std::string& error) {
    logger.error("WebSocket error: " + error);
}

// ==================== 业务逻辑处理 ====================

void WebSocketServer::handleConnect(const std::string& connId, const nlohmann::json& payload) {
    std::string userId = payload.value("userId", "");
    std::string token = payload.value("token", "");
    
    if (userId.empty() || token.empty()) {
        Message errorMsg = MessageFactory::createErrorMessage("AUTH_ERROR", "userId and token are required");
        sendToConnection(connId, errorMsg.toJson());
        return;
    }
    
    // 验证Token
    if (!validateToken(userId, token)) {
        Message errorMsg = MessageFactory::createErrorMessage("AUTH_ERROR", "Invalid token");
        sendToConnection(connId, errorMsg.toJson());
        return;
    }
    
    // 更新连接信息
    {
        std::lock_guard<std::mutex> lock(connectionsMutex);
        auto it = connections.find(connId);
        if (it != connections.end()) {
            it->second.userId = userId;
            it->second.isAuthenticated = true;
            // TODO: 从数据库获取用户名
            it->second.username = "user_" + userId.substr(0, 8);
        }
    }
    
    logger.info("User authenticated: " + userId + " (connection: " + connId + ")");
    
    // 发送认证成功消息
    Message successMsg = MessageFactory::createConnectMessage(userId, "");
    successMsg.payload["status"] = "authenticated";
    {
        std::lock_guard<std::mutex> lock(connectionsMutex);
        auto it = connections.find(connId);
        if (it != connections.end()) {
            successMsg.payload["username"] = it->second.username;
        }
    }
    sendToConnection(connId, successMsg.toJson());
}

void WebSocketServer::handleDisconnect(const std::string& connId, const std::string& reason) {
    std::lock_guard<std::mutex> connLock(connectionsMutex);
    
    auto it = connections.find(connId);
    if (it == connections.end()) {
        return;
    }
    
    std::string userId = it->second.userId;
    std::string roomId = it->second.roomId;
    
    // 如果用户在房间中，离开房间
    if (!roomId.empty()) {
        std::lock_guard<std::mutex> roomLock(roomsMutex);
        auto roomIt = rooms.find(roomId);
        if (roomIt != rooms.end()) {
            roomIt->second.memberIds.erase(connId);
            
            // 通知房间内其他用户
            Message leaveMsg = MessageFactory::createUserLeaveMessage(userId);
            broadcastToRoom(roomId, leaveMsg.toJson(), connId);
            
            // 如果房间为空，删除房间
            if (roomIt->second.memberIds.empty()) {
                rooms.erase(roomIt);
                logger.info("Room removed (empty): " + roomId);
            }
        }
    }
    
    logger.info("User disconnected: " + userId + " (connection: " + connId + ")");
}

void WebSocketServer::handleHeartbeat(const std::string& connId) {
    // 更新心跳时间
    {
        std::lock_guard<std::mutex> lock(connectionsMutex);
        auto it = connections.find(connId);
        if (it != connections.end()) {
            it->second.lastHeartbeat = std::chrono::system_clock::now();
        }
    }
    
    // 发送心跳响应
    Message response = MessageFactory::createHeartbeatResponse();
    sendToConnection(connId, response.toJson());
}

void WebSocketServer::handleJoinRoom(const std::string& connId, const nlohmann::json& payload) {
    std::string roomId = payload.value("roomId", "");
    
    if (roomId.empty()) {
        Message errorMsg = MessageFactory::createErrorMessage("INVALID_REQUEST", "roomId is required");
        sendToConnection(connId, errorMsg.toJson());
        return;
    }
    
    std::lock_guard<std::mutex> connLock(connectionsMutex);
    auto connIt = connections.find(connId);
    
    if (connIt == connections.end()) {
        return;
    }
    
    // 检查是否已认证
    if (!connIt->second.isAuthenticated) {
        Message errorMsg = MessageFactory::createErrorMessage("AUTH_ERROR", "Not authenticated");
        sendToConnection(connId, errorMsg.toJson());
        return;
    }
    
    std::string userId = connIt->second.userId;
    std::string username = connIt->second.username;
    
    // 如果已在其他房间，先离开
    if (!connIt->second.roomId.empty()) {
        handleLeaveRoom(connId, {{"roomId", connIt->second.roomId}});
    }
    
    // 加入房间
    {
        std::lock_guard<std::mutex> roomLock(roomsMutex);
        
        // 创建或获取房间
        auto& room = rooms[roomId];
        room.roomId = roomId;
        room.memberIds.insert(connId);
        
        // 更新连接的房间信息
        connIt->second.roomId = roomId;
        
        // 发送房间信息给新加入的用户
        Message roomInfoMsg = MessageFactory::createRoomInfoMessage(roomId, room.memberIds.size());
        sendToConnection(connId, roomInfoMsg.toJson());
        
        // 通知房间内其他用户
        Message joinMsg = MessageFactory::createUserJoinMessage(userId, username);
        broadcastToRoom(roomId, joinMsg.toJson(), connId);
    }
    
    logger.info("User joined room: " + userId + " -> " + roomId);
}

void WebSocketServer::handleLeaveRoom(const std::string& connId, const nlohmann::json& payload) {
    std::string roomId = payload.value("roomId", "");
    
    if (roomId.empty()) {
        return;
    }
    
    std::lock_guard<std::mutex> connLock(connectionsMutex);
    auto connIt = connections.find(connId);
    
    if (connIt == connections.end() || connIt->second.roomId != roomId) {
        return;
    }
    
    std::string userId = connIt->second.userId;
    
    {
        std::lock_guard<std::mutex> roomLock(roomsMutex);
        auto roomIt = rooms.find(roomId);
        if (roomIt != rooms.end()) {
            roomIt->second.memberIds.erase(connId);
            
            // 通知房间内其他用户
            Message leaveMsg = MessageFactory::createUserLeaveMessage(userId);
            broadcastToRoom(roomId, leaveMsg.toJson(), connId);
            
            // 如果房间为空，删除房间
            if (roomIt->second.memberIds.empty()) {
                rooms.erase(roomIt);
            }
        }
    }
    
    // 清除连接的房间信息
    connIt->second.roomId = "";
    
    logger.info("User left room: " + userId + " <- " + roomId);
}

void WebSocketServer::handleOperation(const std::string& connId, const nlohmann::json& payload) {
    std::lock_guard<std::mutex> lock(connectionsMutex);
    auto it = connections.find(connId);
    
    if (it == connections.end() || it->second.roomId.empty()) {
        Message errorMsg = MessageFactory::createErrorMessage("INVALID_STATE", "Not in a room");
        sendToConnection(connId, errorMsg.toJson());
        return;
    }
    
    std::string roomId = it->second.roomId;
    
    // 广播操作到房间内其他用户
    Message opMsg;
    opMsg.type = MessageType::OPERATION;
    opMsg.messageId = payload.value("opId", "");
    opMsg.timestamp = Message::getCurrentTimestamp();
    opMsg.payload = payload;
    
    broadcastToRoom(roomId, opMsg.toJson(), connId);
    
    // 发送确认消息给操作者
    Message ackMsg = MessageFactory::createOperationAckMessage(
        payload.value("opId", ""),
        payload.value("version", 0)
    );
    sendToConnection(connId, ackMsg.toJson());
}

void WebSocketServer::handleCursorUpdate(const std::string& connId, const nlohmann::json& payload) {
    std::lock_guard<std::mutex> lock(connectionsMutex);
    auto it = connections.find(connId);
    
    if (it == connections.end() || it->second.roomId.empty()) {
        return;
    }
    
    std::string roomId = it->second.roomId;
    std::string userId = it->second.userId;
    
    // 创建光标更新消息
    Message cursorMsg = MessageFactory::createCursorUpdateMessage(
        userId,
        payload.value("position", 0),
        payload.value("selectionStart", 0),
        payload.value("selectionEnd", 0)
    );
    
    // 广播到房间内其他用户
    broadcastToRoom(roomId, cursorMsg.toJson(), connId);
}

// ==================== 连接管理 ====================

void WebSocketServer::addConnection(const std::string& connId, crow::websocket::connection* wsConn) {
    std::lock_guard<std::mutex> lock(connectionsMutex);
    
    WebSocketConnection conn;
    conn.connectionId = connId;
    conn.wsConn = wsConn;
    conn.lastHeartbeat = std::chrono::system_clock::now();
    
    connections[connId] = conn;
}

void WebSocketServer::removeConnection(const std::string& connId) {
    std::lock_guard<std::mutex> lock(connectionsMutex);
    connections.erase(connId);
}

WebSocketConnection* WebSocketServer::getConnection(const std::string& connId) {
    std::lock_guard<std::mutex> lock(connectionsMutex);
    auto it = connections.find(connId);
    return (it != connections.end()) ? &it->second : nullptr;
}

bool WebSocketServer::validateToken(const std::string& userId, const std::string& token) {
    // TODO: 实现真实的Token验证逻辑
    // 目前简单验证：token不为空即可
    // 生产环境应该查询数据库或使用JWT验证
    
    if (token.empty()) {
        return false;
    }
    
    // 模拟验证：token长度大于10视为有效
    return token.length() > 10;
}

std::string WebSocketServer::generateConnectionId() {
    uuid_t uuid;
    uuid_generate(uuid);
    char uuid_str[37];
    uuid_unparse_lower(uuid, uuid_str);
    return std::string(uuid_str);
}

// ==================== 心跳检测 ====================

void WebSocketServer::startHeartbeatChecker() {
    std::thread([this]() {
        while (running) {
            std::this_thread::sleep_for(heartbeatInterval);
            checkTimeoutConnections();
        }
    }).detach();
    
    logger.info("Heartbeat checker started (interval: " + 
                std::to_string(heartbeatInterval.count()) + "s)");
}

void WebSocketServer::checkTimeoutConnections() {
    auto now = std::chrono::system_clock::now();
    std::vector<std::string> timeoutConnections;
    
    {
        std::lock_guard<std::mutex> lock(connectionsMutex);
        
        for (const auto& pair : connections) {
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                now - pair.second.lastHeartbeat);
            
            if (duration > connectionTimeout) {
                timeoutConnections.push_back(pair.first);
            }
        }
    }
    
    // 关闭超时连接
    for (const auto& connId : timeoutConnections) {
        logger.warning("Connection timeout: " + connId);
        
        std::lock_guard<std::mutex> lock(connectionsMutex);
        auto it = connections.find(connId);
        if (it != connections.end() && it->second.wsConn) {
            try {
                it->second.wsConn->close("Connection timeout");
            } catch (...) {
                // 忽略关闭错误
            }
        }
    }
}

} // namespace network

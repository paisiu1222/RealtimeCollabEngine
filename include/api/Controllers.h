#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "storage/Database.h"
#include "storage/DocumentDAO.h"
#include "storage/OperationDAO.h"
#include "utils/Logger.h"

using json = nlohmann::json;
using namespace storage;
using namespace utils;

namespace api {

/**
 * API控制器 - 处理所有HTTP请求
 */
class Controllers {
public:
    /**
     * 初始化控制器
     */
    static void initialize(Database& db);
    
    // ==================== 文档管理API ====================
    
    /**
     * POST /api/documents - 创建文档
     * Request: {"title": "My Document", "owner_id": "user_001"}
     * Response: {"doc_id": "...", "title": "...", "created_at": "..."}
     */
    static std::string createDocument(const std::string& requestBody);
    
    /**
     * GET /api/documents/:id - 获取文档
     * Response: {"doc_id": "...", "title": "...", "content": "...", "version": N}
     */
    static std::string getDocument(const std::string& docId);
    
    /**
     * PUT /api/documents/:id - 更新文档
     * Request: {"title": "New Title", "content": "..."}
     * Response: {"success": true, "version": N}
     */
    static std::string updateDocument(const std::string& docId, const std::string& requestBody);
    
    /**
     * DELETE /api/documents/:id - 删除文档
     * Response: {"success": true}
     */
    static std::string deleteDocument(const std::string& docId);
    
    /**
     * GET /api/documents/:id/history - 获取文档历史
     * Response: {"operations": [...], "count": N}
     */
    static std::string getDocumentHistory(const std::string& docId);
    
    /**
     * GET /api/documents - 列出所有文档
     * Response: {"documents": [...], "total": N}
     */
    static std::string listDocuments();
    
    // ==================== 用户认证API ====================
    
    /**
     * POST /api/auth/register - 注册用户
     * Request: {"username": "john", "email": "john@example.com", "password": "..."}
     * Response: {"user_id": "...", "username": "...", "token": "..."}
     */
    static std::string registerUser(const std::string& requestBody);
    
    /**
     * POST /api/auth/login - 用户登录
     * Request: {"username": "john", "password": "..."}
     * Response: {"user_id": "...", "username": "...", "token": "..."}
     */
    static std::string loginUser(const std::string& requestBody);
    
    /**
     * GET /api/users/:id/profile - 获取用户资料
     * Response: {"user_id": "...", "username": "...", "email": "..."}
     */
    static std::string getUserProfile(const std::string& userId);
    
    // ==================== 房间管理API ====================
    
    /**
     * POST /api/rooms - 创建房间
     * Request: {"name": "Room 1", "doc_id": "...", "max_users": 10}
     * Response: {"room_id": "...", "name": "...", "doc_id": "..."}
     */
    static std::string createRoom(const std::string& requestBody);
    
    /**
     * GET /api/rooms/:id - 获取房间信息
     * Response: {"room_id": "...", "name": "...", "users": [...], "active_count": N}
     */
    static std::string getRoomInfo(const std::string& roomId);
    
    /**
     * POST /api/rooms/:id/join - 加入房间
     * Request: {"user_id": "..."}
     * Response: {"success": true, "room_id": "...", "users": [...]}
     */
    static std::string joinRoom(const std::string& roomId, const std::string& requestBody);
    
    /**
     * POST /api/rooms/:id/leave - 离开房间
     * Request: {"user_id": "..."}
     * Response: {"success": true}
     */
    static std::string leaveRoom(const std::string& roomId, const std::string& requestBody);
    
    // ==================== 健康检查 ====================
    
    /**
     * GET /health - 健康检查
     * Response: {"status": "healthy", "uptime": N, "connections": N}
     */
    static std::string healthCheck();
    
private:
    static Database* database;
    static DocumentDAO* docDAO;
    static OperationDAO* opDAO;
    static Logger& logger;
};

} // namespace api

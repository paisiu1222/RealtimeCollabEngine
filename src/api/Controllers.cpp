#include "api/Controllers.h"
#include <chrono>
#include <sstream>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace api {

// 静态成员初始化
Database* Controllers::database = nullptr;
DocumentDAO* Controllers::docDAO = nullptr;
OperationDAO* Controllers::opDAO = nullptr;
Logger& Controllers::logger = Logger::getInstance();

void Controllers::initialize(Database& db) {
    database = &db;
    docDAO = new DocumentDAO(db);
    opDAO = new OperationDAO(db);
    logger.info("API Controllers initialized");
}

// ==================== 辅助函数 ====================

static std::string generateId() {
    boost::uuids::random_generator gen;
    boost::uuids::uuid uuid = gen();
    return boost::uuids::to_string(uuid);
}

static std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

static json errorResponse(const std::string& message, int code = 400) {
    json resp;
    resp["success"] = false;
    resp["error"] = message;
    resp["code"] = code;
    return resp;
}

static json successResponse(const json& data = json::object()) {
    json resp;
    resp["success"] = true;
    for (auto it = data.begin(); it != data.end(); ++it) {
        resp[it.key()] = it.value();
    }
    return resp;
}

// ==================== 文档管理API ====================

std::string Controllers::createDocument(const std::string& requestBody) {
    try {
        json req = json::parse(requestBody);
        
        if (!req.contains("title") || !req.contains("owner_id")) {
            return errorResponse("Missing required fields: title, owner_id").dump();
        }
        
        std::string docId = generateId();
        std::string title = req["title"];
        std::string ownerId = req["owner_id"];
        
        Document doc;
        doc.docId = docId;
        doc.title = title;
        doc.ownerId = ownerId;
        doc.content = "";
        doc.currentVersion = 0;
        
        if (docDAO->createDocument(doc)) {
            json resp = successResponse();
            resp["doc_id"] = docId;
            resp["title"] = title;
            resp["owner_id"] = ownerId;
            resp["created_at"] = getCurrentTimestamp();
            return resp.dump();
        } else {
            return errorResponse("Failed to create document", 500).dump();
        }
        
    } catch (const std::exception& e) {
        logger.error("CreateDocument error: " + std::string(e.what()));
        return errorResponse("Internal server error", 500).dump();
    }
}

std::string Controllers::getDocument(const std::string& docId) {
    try {
        Document doc = docDAO->getDocument(docId);
        
        if (doc.docId.empty()) {
            return errorResponse("Document not found", 404).dump();
        }
        
        json resp = successResponse();
        resp["doc_id"] = doc.docId;
        resp["title"] = doc.title;
        resp["content"] = doc.content;
        resp["version"] = doc.currentVersion;
        resp["owner_id"] = doc.ownerId;
        resp["created_at"] = doc.createdAt;
        resp["updated_at"] = doc.updatedAt;
        
        return resp.dump();
        
    } catch (const std::exception& e) {
        logger.error("GetDocument error: " + std::string(e.what()));
        return errorResponse("Internal server error", 500).dump();
    }
}

std::string Controllers::updateDocument(const std::string& docId, const std::string& requestBody) {
    try {
        json req = json::parse(requestBody);
        
        Document doc = docDAO->getDocument(docId);
        if (doc.docId.empty()) {
            return errorResponse("Document not found", 404).dump();
        }
        
        if (req.contains("title")) {
            doc.title = req["title"];
        }
        if (req.contains("content")) {
            doc.content = req["content"];
        }
        doc.currentVersion++;
        
        if (docDAO->updateDocument(doc)) {
            json resp = successResponse();
            resp["version"] = doc.currentVersion;
            resp["updated_at"] = getCurrentTimestamp();
            return resp.dump();
        } else {
            return errorResponse("Failed to update document", 500).dump();
        }
        
    } catch (const std::exception& e) {
        logger.error("UpdateDocument error: " + std::string(e.what()));
        return errorResponse("Internal server error", 500).dump();
    }
}

std::string Controllers::deleteDocument(const std::string& docId) {
    try {
        if (docDAO->deleteDocument(docId)) {
            return successResponse().dump();
        } else {
            return errorResponse("Document not found or delete failed", 404).dump();
        }
        
    } catch (const std::exception& e) {
        logger.error("DeleteDocument error: " + std::string(e.what()));
        return errorResponse("Internal server error", 500).dump();
    }
}

std::string Controllers::getDocumentHistory(const std::string& docId) {
    try {
        // 检查文档是否存在
        Document doc = docDAO->getDocument(docId);
        if (doc.docId.empty()) {
            return errorResponse("Document not found", 404).dump();
        }
        
        // 获取所有操作（简化实现，实际应该分页）
        std::vector<Operation> operations = opDAO->getDocumentOperations(docId);
        
        json opsArray = json::array();
        for (const auto& op : operations) {
            json opJson;
            opJson["op_id"] = op.opId;
            opJson["user_id"] = op.userId;
            opJson["version"] = op.version;
            opJson["op_type"] = op.opType;
            opJson["position"] = op.position;
            opJson["content"] = op.content;
            opJson["timestamp"] = op.timestamp;
            opsArray.push_back(opJson);
        }
        
        json resp = successResponse();
        resp["operations"] = opsArray;
        resp["count"] = operations.size();
        
        return resp.dump();
        
    } catch (const std::exception& e) {
        logger.error("GetDocumentHistory error: " + std::string(e.what()));
        return errorResponse("Internal server error", 500).dump();
    }
}

std::string Controllers::listDocuments() {
    try {
        // 简化实现：返回空列表
        // TODO: 实现完整的文档列表查询
        json resp = successResponse();
        resp["documents"] = json::array();
        resp["total"] = 0;
        return resp.dump();
        
    } catch (const std::exception& e) {
        logger.error("ListDocuments error: " + std::string(e.what()));
        return errorResponse("Internal server error", 500).dump();
    }
}

// ==================== 用户认证API ====================

std::string Controllers::registerUser(const std::string& requestBody) {
    try {
        json req = json::parse(requestBody);
        
        if (!req.contains("username") || !req.contains("email") || !req.contains("password")) {
            return errorResponse("Missing required fields: username, email, password").dump();
        }
        
        std::string userId = generateId();
        std::string username = req["username"];
        std::string email = req["email"];
        
        // TODO: 实际应该将用户存储到数据库
        // 这里简化处理，直接返回成功
        
        json resp = successResponse();
        resp["user_id"] = userId;
        resp["username"] = username;
        resp["email"] = email;
        resp["token"] = generateId(); // 简化：使用UUID作为token
        
        logger.info("User registered: " + username);
        return resp.dump();
        
    } catch (const std::exception& e) {
        logger.error("RegisterUser error: " + std::string(e.what()));
        return errorResponse("Internal server error", 500).dump();
    }
}

std::string Controllers::loginUser(const std::string& requestBody) {
    try {
        json req = json::parse(requestBody);
        
        if (!req.contains("username") || !req.contains("password")) {
            return errorResponse("Missing required fields: username, password").dump();
        }
        
        std::string username = req["username"];
        
        // TODO: 实际应该验证用户名和密码
        // 这里简化处理，直接返回成功
        
        std::string userId = generateId();
        
        json resp = successResponse();
        resp["user_id"] = userId;
        resp["username"] = username;
        resp["token"] = generateId();
        
        logger.info("User logged in: " + username);
        return resp.dump();
        
    } catch (const std::exception& e) {
        logger.error("LoginUser error: " + std::string(e.what()));
        return errorResponse("Internal server error", 500).dump();
    }
}

std::string Controllers::getUserProfile(const std::string& userId) {
    try {
        // TODO: 从数据库获取用户资料
        // 这里返回模拟数据
        
        json resp = successResponse();
        resp["user_id"] = userId;
        resp["username"] = "demo_user";
        resp["email"] = "demo@example.com";
        
        return resp.dump();
        
    } catch (const std::exception& e) {
        logger.error("GetUserProfile error: " + std::string(e.what()));
        return errorResponse("Internal server error", 500).dump();
    }
}

// ==================== 房间管理API ====================

std::string Controllers::createRoom(const std::string& requestBody) {
    try {
        json req = json::parse(requestBody);
        
        if (!req.contains("name") || !req.contains("doc_id")) {
            return errorResponse("Missing required fields: name, doc_id").dump();
        }
        
        std::string roomId = generateId();
        std::string name = req["name"];
        std::string docId = req["doc_id"];
        int maxUsers = req.value("max_users", 10);
        
        // TODO: 实际应该将房间存储到数据库
        // 这里简化处理
        
        json resp = successResponse();
        resp["room_id"] = roomId;
        resp["name"] = name;
        resp["doc_id"] = docId;
        resp["max_users"] = maxUsers;
        resp["created_at"] = getCurrentTimestamp();
        
        logger.info("Room created: " + name);
        return resp.dump();
        
    } catch (const std::exception& e) {
        logger.error("CreateRoom error: " + std::string(e.what()));
        return errorResponse("Internal server error", 500).dump();
    }
}

std::string Controllers::getRoomInfo(const std::string& roomId) {
    try {
        // TODO: 从数据库获取房间信息
        // 这里返回模拟数据
        
        json resp = successResponse();
        resp["room_id"] = roomId;
        resp["name"] = "Demo Room";
        resp["doc_id"] = "doc_demo";
        resp["users"] = json::array();
        resp["active_count"] = 0;
        
        return resp.dump();
        
    } catch (const std::exception& e) {
        logger.error("GetRoomInfo error: " + std::string(e.what()));
        return errorResponse("Internal server error", 500).dump();
    }
}

std::string Controllers::joinRoom(const std::string& roomId, const std::string& requestBody) {
    try {
        json req = json::parse(requestBody);
        
        if (!req.contains("user_id")) {
            return errorResponse("Missing required field: user_id").dump();
        }
        
        std::string userId = req["user_id"];
        
        // TODO: 实际应该更新房间用户列表
        // 这里简化处理
        
        json resp = successResponse();
        resp["room_id"] = roomId;
        resp["user_id"] = userId;
        
        logger.info("User joined room: " + userId + " -> " + roomId);
        return resp.dump();
        
    } catch (const std::exception& e) {
        logger.error("JoinRoom error: " + std::string(e.what()));
        return errorResponse("Internal server error", 500).dump();
    }
}

std::string Controllers::leaveRoom(const std::string& roomId, const std::string& requestBody) {
    try {
        json req = json::parse(requestBody);
        
        if (!req.contains("user_id")) {
            return errorResponse("Missing required field: user_id").dump();
        }
        
        std::string userId = req["user_id"];
        
        // TODO: 实际应该从房间用户列表移除
        // 这里简化处理
        
        json resp = successResponse();
        resp["room_id"] = roomId;
        resp["user_id"] = userId;
        
        logger.info("User left room: " + userId + " <- " + roomId);
        return resp.dump();
        
    } catch (const std::exception& e) {
        logger.error("LeaveRoom error: " + std::string(e.what()));
        return errorResponse("Internal server error", 500).dump();
    }
}

// ==================== 健康检查 ====================

std::string Controllers::healthCheck() {
    json resp;
    resp["status"] = "healthy";
    resp["uptime"] = 0; // TODO: 计算实际运行时间
    resp["connections"] = 0; // TODO: 获取实际连接数
    resp["version"] = "1.0.0";
    return resp.dump();
}

} // namespace api

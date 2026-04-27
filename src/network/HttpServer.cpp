#include "network/HttpServer.h"

namespace network {

HttpServer::HttpServer(const std::string& host, int port, Database& db)
    : host(host), port(port), database(db), logger(Logger::getInstance()) {
    
    // 初始化API控制器
    Controllers::initialize(database);
    
    logger.info("HTTP Server created on " + host + ":" + std::to_string(port));
}

void HttpServer::start() {
    setupRoutes();
    
    logger.info("Starting HTTP server on " + host + ":" + std::to_string(port));
    
    // Crow服务器启动（阻塞）
    app.bindaddr(host).port(port).run();
}

void HttpServer::stop() {
    logger.info("Stopping HTTP server");
    // Crow没有提供优雅的停止方法，应用退出时会自动清理
}

void HttpServer::setupRoutes() {
    logger.info("Setting up HTTP routes");
    
    // ==================== 健康检查 ====================
    CROW_ROUTE(app, "/health")
    ([]() {
        return Controllers::healthCheck();
    });
    
    // ==================== 文档管理API ====================
    
    // POST /api/documents - 创建文档
    CROW_ROUTE(app, "/api/documents").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {
        return Controllers::createDocument(req.body);
    });
    
    // GET /api/documents - 列出所有文档
    CROW_ROUTE(app, "/api/documents").methods(crow::HTTPMethod::GET)
    ([]() {
        return Controllers::listDocuments();
    });
    
    // GET /api/documents/:id - 获取文档
    CROW_ROUTE(app, "/api/documents/<string>").methods(crow::HTTPMethod::GET)
    ([](const std::string& docId) {
        return Controllers::getDocument(docId);
    });
    
    // PUT /api/documents/:id - 更新文档
    CROW_ROUTE(app, "/api/documents/<string>").methods(crow::HTTPMethod::PUT)
    ([](const crow::request& req, const std::string& docId) {
        return Controllers::updateDocument(docId, req.body);
    });
    
    // DELETE /api/documents/:id - 删除文档
    CROW_ROUTE(app, "/api/documents/<string>").methods(crow::HTTPMethod::DELETE)
    ([](const std::string& docId) {
        return Controllers::deleteDocument(docId);
    });
    
    // GET /api/documents/:id/history - 获取文档历史
    CROW_ROUTE(app, "/api/documents/<string>/history").methods(crow::HTTPMethod::GET)
    ([](const std::string& docId) {
        return Controllers::getDocumentHistory(docId);
    });
    
    // ==================== 用户认证API ====================
    
    // POST /api/auth/register - 注册用户
    CROW_ROUTE(app, "/api/auth/register").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {
        return Controllers::registerUser(req.body);
    });
    
    // POST /api/auth/login - 用户登录
    CROW_ROUTE(app, "/api/auth/login").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {
        return Controllers::loginUser(req.body);
    });
    
    // GET /api/users/:id/profile - 获取用户资料
    CROW_ROUTE(app, "/api/users/<string>/profile").methods(crow::HTTPMethod::GET)
    ([](const std::string& userId) {
        return Controllers::getUserProfile(userId);
    });
    
    // ==================== 房间管理API ====================
    
    // POST /api/rooms - 创建房间
    CROW_ROUTE(app, "/api/rooms").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {
        return Controllers::createRoom(req.body);
    });
    
    // GET /api/rooms/:id - 获取房间信息
    CROW_ROUTE(app, "/api/rooms/<string>").methods(crow::HTTPMethod::GET)
    ([](const std::string& roomId) {
        return Controllers::getRoomInfo(roomId);
    });
    
    // POST /api/rooms/:id/join - 加入房间
    CROW_ROUTE(app, "/api/rooms/<string>/join").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req, const std::string& roomId) {
        return Controllers::joinRoom(roomId, req.body);
    });
    
    // POST /api/rooms/:id/leave - 离开房间
    CROW_ROUTE(app, "/api/rooms/<string>/leave").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req, const std::string& roomId) {
        return Controllers::leaveRoom(roomId, req.body);
    });
    
    logger.info("HTTP routes configured successfully");
}

} // namespace network

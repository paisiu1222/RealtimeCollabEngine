#pragma once

#include <string>
#include <functional>
#include <crow.h>
#include "api/Controllers.h"
#include "storage/Database.h"
#include "utils/Logger.h"

using namespace api;
using namespace storage;
using namespace utils;

namespace network {

/**
 * HTTP服务器 - 提供RESTful API服务
 */
class HttpServer {
public:
    /**
     * 构造函数
     * @param host 监听地址
     * @param port 监听端口
     * @param db 数据库引用
     */
    HttpServer(const std::string& host, int port, Database& db);
    
    /**
     * 启动HTTP服务器（阻塞）
     */
    void start();
    
    /**
     * 停止HTTP服务器
     */
    void stop();
    
private:
    /**
     * 注册所有路由
     */
    void setupRoutes();
    
    std::string host;
    int port;
    Database& database;
    crow::SimpleApp app;
    Logger& logger;
};

} // namespace network

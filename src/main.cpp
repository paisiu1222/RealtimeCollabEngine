#include <iostream>
#include <string>
#include <thread>
#include <chrono>

// 包含项目头文件
#include "utils/Logger.h"
#include "utils/Config.h"
#include "storage/Database.h"
#include "storage/DocumentDAO.h"
#include "storage/OperationDAO.h"

// 包含第三方库
#include <nlohmann/json.hpp>
#include <crow.h>
#include <boost/version.hpp>
#include <sqlite3.h>

using json = nlohmann::json;
using namespace utils;
using namespace storage;

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  RealtimeCollabEngine v1.0.0" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // 初始化日志系统
    auto& logger = Logger::getInstance();
    
    // 创建日志目录（如果不存在）
    system("mkdir -p logs");
    
    logger.initialize("logs/server.log", LogLevel::DEBUG, true, false);
    
    LOG_INFO("Starting RealtimeCollabEngine");
    LOG_DEBUG("Debug mode enabled");
    
    // 测试Boost版本
    std::string boostVersion = std::to_string(BOOST_VERSION / 100000) + "." +
                               std::to_string(BOOST_VERSION / 100 % 1000) + "." +
                               std::to_string(BOOST_VERSION % 100);
    LOG_INFO("Boost Version: " + boostVersion);
    
    // 测试SQLite3版本
    LOG_INFO(std::string("SQLite3 Version: ") + sqlite3_libversion());
    
    // 测试nlohmann/json
    json testJson = {
        {"message", "JSON library working"},
        {"status", "success"}
    };
    LOG_INFO("nlohmann/json: " + testJson.dump());
    
    // 测试Crow
    LOG_INFO("Crow framework: Available");
    
    // 测试配置系统
    auto& config = Config::getInstance();
    
    // 尝试加载配置文件
    if (config.loadFromFile("config.json")) {
        LOG_INFO("Configuration loaded successfully");
        LOG_INFO("Server Host: " + config.getServerHost());
        LOG_INFO("Server Port: " + std::to_string(config.getServerPort()));
        LOG_INFO("Max Connections: " + std::to_string(config.getMaxConnections()));
        LOG_INFO("Database Path: " + config.getDatabasePath());
    } else {
        LOG_WARNING("Failed to load config.json, using defaults");
        LOG_WARNING("Error: " + config.getLastError());
    }
    
    // 测试配置设置
    config.set("test.key", "test_value");
    LOG_INFO("Test config value: " + config.get<std::string>("test.key", ""));
    
    // ============================================
    // 测试数据库层
    // ============================================
    LOG_INFO("=== Testing Database Layer ===");
    
    // 初始化数据库
    DatabaseConfig dbConfig;
    dbConfig.dbPath = "test_collab_engine.db";
    dbConfig.poolSize = 3;
    
    auto& database = Database::getInstance();
    if (database.initialize(dbConfig)) {
        LOG_INFO("Database initialized successfully");
        LOG_INFO("Database Path: " + database.getDbPath());
        LOG_INFO("Connection Pool Size: " + std::to_string(database.getPoolSize()));
    } else {
        LOG_ERROR("Failed to initialize database");
        return 1;
    }
    
    // 测试DocumentDAO
    LOG_INFO("Testing DocumentDAO...");
    DocumentDAO docDAO(database);
    
    // 创建测试文档
    Document testDoc;
    testDoc.docId = "doc_001";
    testDoc.title = "Test Document";
    testDoc.ownerId = "user_001";
    testDoc.content = "Hello, World!";
    testDoc.currentVersion = 0;
    
    if (docDAO.createDocument(testDoc)) {
        LOG_INFO("Document created successfully: " + testDoc.docId);
    } else {
        LOG_ERROR("Failed to create document");
    }
    
    // 获取文档
    Document retrievedDoc = docDAO.getDocument("doc_001");
    if (!retrievedDoc.docId.empty()) {
        LOG_INFO("Document retrieved: " + retrievedDoc.title);
        LOG_INFO("Content: " + retrievedDoc.content);
        LOG_INFO("Version: " + std::to_string(retrievedDoc.currentVersion));
    } else {
        LOG_ERROR("Failed to retrieve document");
    }
    
    // 更新文档
    retrievedDoc.title = "Updated Test Document";
    retrievedDoc.content = "Hello, Updated World!";
    retrievedDoc.currentVersion = 1;
    
    if (docDAO.updateDocument(retrievedDoc)) {
        LOG_INFO("Document updated successfully");
    } else {
        LOG_ERROR("Failed to update document");
    }
    
    // 检查文档是否存在
    if (docDAO.exists("doc_001")) {
        LOG_INFO("Document exists check passed");
    }
    
    // 测试OperationDAO
    LOG_INFO("Testing OperationDAO...");
    OperationDAO opDAO(database);
    
    // 创建测试操作
    Operation testOp;
    testOp.docId = "doc_001";
    testOp.opId = "op_001";
    testOp.userId = "user_001";
    testOp.version = 1;
    testOp.opType = "insert";
    testOp.position = 0;
    testOp.length = 5;
    testOp.content = "Hello";
    
    if (opDAO.insertOperation(testOp)) {
        LOG_INFO("Operation inserted successfully: " + testOp.opId);
    } else {
        LOG_ERROR("Failed to insert operation");
    }
    
    // 获取操作
    Operation retrievedOp = opDAO.getOperation("op_001");
    if (!retrievedOp.opId.empty()) {
        LOG_INFO("Operation retrieved: type=" + retrievedOp.opType);
        LOG_INFO("Content: " + retrievedOp.content);
    }
    
    // 获取当前版本
    int currentVersion = opDAO.getCurrentVersion("doc_001");
    LOG_INFO("Current version: " + std::to_string(currentVersion));
    
    // 获取操作数量
    int opCount = opDAO.getOperationCount("doc_001");
    LOG_INFO("Operation count: " + std::to_string(opCount));
    
    // 显示日志统计
    auto stats = logger.getStats();
    LOG_INFO("Log Statistics - Total: " + std::to_string(stats.totalLogs) +
             ", Info: " + std::to_string(stats.infoCount) +
             ", Debug: " + std::to_string(stats.debugCount));
    
    std::cout << std::endl;
    LOG_INFO("All dependencies loaded successfully!");
    LOG_INFO("System initialization complete");
    
    std::cout << "========================================" << std::endl;
    
    // 关闭数据库
    database.shutdown();
    
    // 关闭日志系统
    logger.shutdown();
    
    // 清理测试数据库
    std::remove("test_collab_engine.db");
    std::remove("test_collab_engine.db-wal");
    std::remove("test_collab_engine.db-shm");
    
    return 0;
}

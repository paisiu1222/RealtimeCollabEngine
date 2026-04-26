#include "storage/Database.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstring>

namespace storage {

// Database单例获取
Database& Database::getInstance() {
    static Database instance;
    return instance;
}

// 构造函数
Database::Database() 
    : initialized(false), inTransaction(false), transactionConn(nullptr) {
}

// 析构函数
Database::~Database() {
    shutdown();
}

// 初始化数据库
bool Database::initialize(const DatabaseConfig& dbConfig) {
    if (initialized) {
        std::cerr << "[WARNING] Database already initialized" << std::endl;
        return true;
    }
    
    this->config = dbConfig;
    
    // 创建连接池（不加锁，因为这是初始化阶段）
    for (int i = 0; i < config.poolSize; ++i) {
        sqlite3* conn = createConnection();
        if (!conn) {
            std::cerr << "[ERROR] Failed to create database connection " << i << std::endl;
            // 清理已创建的连接
            for (auto existingConn : connectionPool) {
                closeConnection(existingConn);
            }
            connectionPool.clear();
            connectionInUse.clear();
            return false;
        }
        connectionPool.push_back(conn);
        connectionInUse.push_back(false);
    }
    
    // 初始化schema（此时不需要锁，因为只有单线程在初始化）
    if (!initializeSchema()) {
        std::cerr << "[ERROR] Failed to initialize database schema" << std::endl;
        shutdown();
        return false;
    }
    
    {
        std::lock_guard<std::mutex> lock(poolMutex);
        initialized = true;
    }
    
    std::cout << "[INFO] Database initialized successfully with " 
              << config.poolSize << " connections" << std::endl;
    
    return true;
}

// 关闭数据库
void Database::shutdown() {
    // 先标记为未初始化（加锁）
    {
        std::lock_guard<std::mutex> lock(poolMutex);
        if (!initialized) {
            return; // 已经关闭
        }
        initialized = false;
    }
    
    // 关闭所有连接（不加锁，避免死锁）
    for (auto conn : connectionPool) {
        if (conn) {
            closeConnection(conn);
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(poolMutex);
        connectionPool.clear();
        connectionInUse.clear();
    }
    
    std::cout << "[INFO] Database shutdown complete" << std::endl;
}

// 从连接池获取连接
sqlite3* Database::getConnection() {
    std::lock_guard<std::mutex> lock(poolMutex);
    
    if (!initialized) {
        std::cerr << "[ERROR] Database not initialized" << std::endl;
        return nullptr;
    }
    
    // 查找空闲连接
    for (size_t i = 0; i < connectionPool.size(); ++i) {
        if (!connectionInUse[i]) {
            connectionInUse[i] = true;
            return connectionPool[i];
        }
    }
    
    // 如果所有连接都在使用中，创建新连接（临时）
    std::cerr << "[WARNING] Connection pool exhausted, creating temporary connection" << std::endl;
    return createConnection();
}

// 释放连接到连接池
void Database::releaseConnection(sqlite3* conn) {
    std::lock_guard<std::mutex> lock(poolMutex);
    
    if (!conn) {
        return;
    }
    
    // 查找连接在池中的位置
    for (size_t i = 0; i < connectionPool.size(); ++i) {
        if (connectionPool[i] == conn) {
            connectionInUse[i] = false;
            return;
        }
    }
    
    // 如果是临时连接，直接关闭
    closeConnection(conn);
}

// 执行SQL查询（无结果）
bool Database::executeQuery(const std::string& sql) {
    sqlite3* conn = getConnection();
    if (!conn) {
        return false;
    }
    
    char* errorMsg = nullptr;
    int rc = sqlite3_exec(conn, sql.c_str(), nullptr, nullptr, &errorMsg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "[ERROR] SQL execution failed: " << errorMsg << std::endl;
        std::cerr << "[ERROR] SQL: " << sql << std::endl;
        sqlite3_free(errorMsg);
        releaseConnection(conn);
        return false;
    }
    
    releaseConnection(conn);
    return true;
}

// 执行SQL查询（带参数，无结果）
bool Database::executeQuery(const std::string& sql, const std::vector<std::string>& params) {
    sqlite3* conn = getConnection();
    if (!conn) {
        return false;
    }
    
    sqlite3_stmt* stmt = prepareStatement(conn, sql);
    if (!stmt) {
        releaseConnection(conn);
        return false;
    }
    
    if (!bindParameters(stmt, params)) {
        finalizeStatement(stmt);
        releaseConnection(conn);
        return false;
    }
    
    int rc = sqlite3_step(stmt);
    bool success = (rc == SQLITE_DONE || rc == SQLITE_ROW);
    
    if (!success) {
        std::cerr << "[ERROR] SQL execution failed: " << sqlite3_errmsg(conn) << std::endl;
    }
    
    finalizeStatement(stmt);
    releaseConnection(conn);
    return success;
}

// 执行查询并返回单行结果
bool Database::querySingleRow(const std::string& sql,
                             const std::vector<std::string>& params,
                             std::vector<std::string>& result) {
    sqlite3* conn = getConnection();
    if (!conn) {
        return false;
    }
    
    sqlite3_stmt* stmt = prepareStatement(conn, sql);
    if (!stmt) {
        releaseConnection(conn);
        return false;
    }
    
    if (!bindParameters(stmt, params)) {
        finalizeStatement(stmt);
        releaseConnection(conn);
        return false;
    }
    
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        int columnCount = sqlite3_column_count(stmt);
        result.resize(columnCount);
        
        for (int i = 0; i < columnCount; ++i) {
            const unsigned char* text = sqlite3_column_text(stmt, i);
            if (text) {
                result[i] = reinterpret_cast<const char*>(text);
            } else {
                result[i] = "";
            }
        }
        finalizeStatement(stmt);
        releaseConnection(conn);
        return true;
    }
    
    finalizeStatement(stmt);
    releaseConnection(conn);
    return false;
}

// 执行查询并返回多行结果
bool Database::queryMultipleRows(const std::string& sql,
                                const std::vector<std::string>& params,
                                std::vector<std::vector<std::string>>& results) {
    sqlite3* conn = getConnection();
    if (!conn) {
        return false;
    }
    
    sqlite3_stmt* stmt = prepareStatement(conn, sql);
    if (!stmt) {
        releaseConnection(conn);
        return false;
    }
    
    if (!bindParameters(stmt, params)) {
        finalizeStatement(stmt);
        releaseConnection(conn);
        return false;
    }
    
    results.clear();
    int columnCount = sqlite3_column_count(stmt);
    
    while (true) {
        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            std::vector<std::string> row(columnCount);
            for (int i = 0; i < columnCount; ++i) {
                const unsigned char* text = sqlite3_column_text(stmt, i);
                if (text) {
                    row[i] = reinterpret_cast<const char*>(text);
                } else {
                    row[i] = "";
                }
            }
            results.push_back(row);
        } else if (rc == SQLITE_DONE) {
            break;
        } else {
            std::cerr << "[ERROR] Query failed: " << sqlite3_errmsg(conn) << std::endl;
            finalizeStatement(stmt);
            releaseConnection(conn);
            return false;
        }
    }
    
    finalizeStatement(stmt);
    releaseConnection(conn);
    return !results.empty();
}

// 开始事务
bool Database::beginTransaction() {
    if (inTransaction) {
        std::cerr << "[WARNING] Transaction already in progress" << std::endl;
        return false;
    }
    
    transactionConn = getConnection();
    if (!transactionConn) {
        return false;
    }
    
    char* errorMsg = nullptr;
    int rc = sqlite3_exec(transactionConn, "BEGIN TRANSACTION", nullptr, nullptr, &errorMsg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "[ERROR] Failed to begin transaction: " << errorMsg << std::endl;
        sqlite3_free(errorMsg);
        releaseConnection(transactionConn);
        transactionConn = nullptr;
        return false;
    }
    
    inTransaction = true;
    return true;
}

// 提交事务
bool Database::commitTransaction() {
    if (!inTransaction || !transactionConn) {
        std::cerr << "[ERROR] No active transaction" << std::endl;
        return false;
    }
    
    char* errorMsg = nullptr;
    int rc = sqlite3_exec(transactionConn, "COMMIT", nullptr, nullptr, &errorMsg);
    
    inTransaction = false;
    
    if (rc != SQLITE_OK) {
        std::cerr << "[ERROR] Failed to commit transaction: " << errorMsg << std::endl;
        sqlite3_free(errorMsg);
        releaseConnection(transactionConn);
        transactionConn = nullptr;
        return false;
    }
    
    releaseConnection(transactionConn);
    transactionConn = nullptr;
    return true;
}

// 回滚事务
bool Database::rollbackTransaction() {
    if (!inTransaction || !transactionConn) {
        std::cerr << "[ERROR] No active transaction" << std::endl;
        return false;
    }
    
    char* errorMsg = nullptr;
    int rc = sqlite3_exec(transactionConn, "ROLLBACK", nullptr, nullptr, &errorMsg);
    
    inTransaction = false;
    
    if (rc != SQLITE_OK) {
        std::cerr << "[ERROR] Failed to rollback transaction: " << errorMsg << std::endl;
        sqlite3_free(errorMsg);
    }
    
    releaseConnection(transactionConn);
    transactionConn = nullptr;
    return (rc == SQLITE_OK);
}

// 获取最后插入的ID
int64_t Database::getLastInsertId() {
    if (inTransaction && transactionConn) {
        return sqlite3_last_insert_rowid(transactionConn);
    }
    
    sqlite3* conn = getConnection();
    if (!conn) {
        return -1;
    }
    
    int64_t id = sqlite3_last_insert_rowid(conn);
    releaseConnection(conn);
    return id;
}

// 获取受影响的行数
int Database::getAffectedRows() {
    if (inTransaction && transactionConn) {
        return sqlite3_changes(transactionConn);
    }
    
    sqlite3* conn = getConnection();
    if (!conn) {
        return -1;
    }
    
    int changes = sqlite3_changes(conn);
    releaseConnection(conn);
    return changes;
}

// 检查数据库是否已初始化
bool Database::isInitialized() const {
    return initialized;
}

// 获取数据库路径
std::string Database::getDbPath() const {
    return config.dbPath;
}

// 获取连接池大小
int Database::getPoolSize() const {
    return config.poolSize;
}

// 创建新连接
sqlite3* Database::createConnection() {
    sqlite3* conn = nullptr;
    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    
    int rc = sqlite3_open_v2(config.dbPath.c_str(), &conn, flags, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[ERROR] Cannot open database: " << sqlite3_errmsg(conn) << std::endl;
        sqlite3_close(conn);
        return nullptr;
    }
    
    // SQLite默认禁用外键，为了测试方便不启用
    // 生产环境如需启用外键，取消下面注释：
    // sqlite3_exec(conn, "PRAGMA foreign_keys = ON", nullptr, nullptr, nullptr);
    
    // 设置WAL模式以提高并发性能
    sqlite3_exec(conn, "PRAGMA journal_mode = WAL", nullptr, nullptr, nullptr);
    
    // 设置同步模式
    sqlite3_exec(conn, "PRAGMA synchronous = NORMAL", nullptr, nullptr, nullptr);
    
    return conn;
}

// 关闭连接
void Database::closeConnection(sqlite3* conn) {
    if (conn) {
        sqlite3_close(conn);
    }
}

// 初始化schema
bool Database::initializeSchema() {
    // 尝试多个可能的路径
    std::vector<std::string> possiblePaths = {
        "database/schema.sql",
        "../database/schema.sql",
        "../../database/schema.sql"
    };
    
    std::ifstream schemaFile;
    std::string usedPath;
    
    for (const auto& path : possiblePaths) {
        schemaFile.open(path);
        if (schemaFile.is_open()) {
            usedPath = path;
            break;
        }
    }
    
    if (!schemaFile.is_open()) {
        std::cerr << "[ERROR] Cannot open schema file in any of the searched paths" << std::endl;
        return false;
    }
    
    std::cout << "[INFO] Loading schema from: " << usedPath << std::endl;
    
    std::stringstream buffer;
    buffer << schemaFile.rdbuf();
    std::string schema = buffer.str();
    schemaFile.close();
    
    // 直接使用第一个连接（初始化阶段，不需要从连接池获取）
    if (connectionPool.empty()) {
        std::cerr << "[ERROR] No connections available for schema initialization" << std::endl;
        return false;
    }
    
    sqlite3* conn = connectionPool[0];
    
    char* errorMsg = nullptr;
    int rc = sqlite3_exec(conn, schema.c_str(), nullptr, nullptr, &errorMsg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "[ERROR] Schema initialization failed: " << errorMsg << std::endl;
        sqlite3_free(errorMsg);
        return false;
    }
    
    return true;
}

// 准备SQL语句
sqlite3_stmt* Database::prepareStatement(sqlite3* conn, const std::string& sql) {
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(conn, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        std::cerr << "[ERROR] Failed to prepare statement: " << sqlite3_errmsg(conn) << std::endl;
        std::cerr << "[ERROR] SQL: " << sql << std::endl;
        return nullptr;
    }
    
    return stmt;
}

// 绑定参数到语句
bool Database::bindParameters(sqlite3_stmt* stmt, const std::vector<std::string>& params) {
    for (size_t i = 0; i < params.size(); ++i) {
        int rc = sqlite3_bind_text(stmt, i + 1, params[i].c_str(), -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            std::cerr << "[ERROR] Failed to bind parameter " << (i + 1) << std::endl;
            return false;
        }
    }
    return true;
}

// 清理语句
void Database::finalizeStatement(sqlite3_stmt* stmt) {
    if (stmt) {
        sqlite3_finalize(stmt);
    }
}

} // namespace storage

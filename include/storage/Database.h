#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <sqlite3.h>

namespace storage {

// 数据库配置结构
struct DatabaseConfig {
    std::string dbPath;
    int poolSize = 5;
    int maxRetries = 3;
    int connectionTimeout = 30; // seconds
};

class Database {
public:
    static Database& getInstance();
    
    // 初始化数据库
    bool initialize(const DatabaseConfig& config);
    
    // 关闭数据库
    void shutdown();
    
    // 从连接池获取连接
    sqlite3* getConnection();
    
    // 释放连接到连接池
    void releaseConnection(sqlite3* conn);
    
    // 执行SQL查询（无结果）
    bool executeQuery(const std::string& sql);
    
    // 执行SQL查询（带参数，无结果）
    bool executeQuery(const std::string& sql, const std::vector<std::string>& params);
    
    // 执行查询并返回单行结果
    bool querySingleRow(const std::string& sql, 
                       const std::vector<std::string>& params,
                       std::vector<std::string>& result);
    
    // 执行查询并返回多行结果
    bool queryMultipleRows(const std::string& sql,
                          const std::vector<std::string>& params,
                          std::vector<std::vector<std::string>>& results);
    
    // 执行事务
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    
    // 获取最后插入的ID
    int64_t getLastInsertId();
    
    // 获取受影响的行数
    int getAffectedRows();
    
    // 检查数据库是否已初始化
    bool isInitialized() const;
    
    // 获取数据库路径
    std::string getDbPath() const;
    
    // 获取连接池大小
    int getPoolSize() const;
    
private:
    Database();
    ~Database();
    
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    
    // 创建新连接
    sqlite3* createConnection();
    
    // 关闭连接
    void closeConnection(sqlite3* conn);
    
    // 初始化schema
    bool initializeSchema();
    
    // 准备SQL语句
    sqlite3_stmt* prepareStatement(sqlite3* conn, const std::string& sql);
    
    // 绑定参数到语句
    bool bindParameters(sqlite3_stmt* stmt, const std::vector<std::string>& params);
    
    // 清理语句
    void finalizeStatement(sqlite3_stmt* stmt);
    
    mutable std::mutex poolMutex;
    std::vector<sqlite3*> connectionPool;
    std::vector<bool> connectionInUse;
    
    DatabaseConfig config;
    bool initialized;
    
    // 当前事务状态
    bool inTransaction;
    sqlite3* transactionConn;
};

} // namespace storage

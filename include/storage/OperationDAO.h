#pragma once

#include <string>
#include <vector>
#include "storage/Database.h"

namespace storage {

// 操作数据结构
struct Operation {
    int64_t id = 0;
    std::string docId;
    std::string opId;
    std::string userId;
    int version = 0;
    std::string opType; // insert, delete, replace
    int position = 0;
    int length = 0;
    std::string content;
    std::string timestamp;
    bool applied = false;
};

class OperationDAO {
public:
    explicit OperationDAO(Database& db);
    
    // 插入操作记录
    bool insertOperation(const Operation& op);
    
    // 获取操作记录
    Operation getOperation(const std::string& opId);
    
    // 获取文档的所有操作（按版本排序）
    std::vector<Operation> getDocumentOperations(const std::string& docId);
    
    // 获取文档指定版本范围的操作
    std::vector<Operation> getOperationsByVersion(const std::string& docId, 
                                                  int startVersion, 
                                                  int endVersion);
    
    // 获取未应用的操作
    std::vector<Operation> getUnappliedOperations(const std::string& docId);
    
    // 标记操作为已应用
    bool markOperationApplied(const std::string& opId);
    
    // 批量标记操作为已应用
    bool markOperationsApplied(const std::vector<std::string>& opIds);
    
    // 删除操作记录
    bool deleteOperation(const std::string& opId);
    
    // 清理旧操作（保留最近的N条）
    bool cleanupOldOperations(const std::string& docId, int keepCount);
    
    // 获取文档的当前版本号
    int getCurrentVersion(const std::string& docId);
    
    // 检查操作ID是否存在
    bool exists(const std::string& opId);
    
    // 获取操作数量
    int getOperationCount(const std::string& docId);
    
private:
    Database& database;
    
    // 从查询结果行构建Operation对象
    Operation buildOperationFromRow(const std::vector<std::string>& row);
};

} // namespace storage

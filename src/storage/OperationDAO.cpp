#include "storage/OperationDAO.h"
#include <iostream>
#include <sstream>

namespace storage {

// 构造函数
OperationDAO::OperationDAO(Database& db) : database(db) {
}

// 插入操作记录
bool OperationDAO::insertOperation(const Operation& op) {
    std::string sql = R"(
        INSERT INTO operations (doc_id, op_id, user_id, version, op_type, 
                               position, length, content, applied)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";
    
    std::vector<std::string> params = {
        op.docId,
        op.opId,
        op.userId,
        std::to_string(op.version),
        op.opType,
        std::to_string(op.position),
        std::to_string(op.length),
        op.content,
        op.applied ? "1" : "0"
    };
    
    return database.executeQuery(sql, params);
}

// 获取操作记录
Operation OperationDAO::getOperation(const std::string& opId) {
    Operation op;
    op.opId = opId;
    
    std::string sql = R"(
        SELECT id, doc_id, op_id, user_id, version, op_type, 
               position, length, content, timestamp, applied
        FROM operations
        WHERE op_id = ?
    )";
    
    std::vector<std::string> params = {opId};
    std::vector<std::string> result;
    
    if (database.querySingleRow(sql, params, result)) {
        op = buildOperationFromRow(result);
    }
    
    return op;
}

// 获取文档的所有操作（按版本排序）
std::vector<Operation> OperationDAO::getDocumentOperations(const std::string& docId) {
    std::vector<Operation> operations;
    
    std::string sql = R"(
        SELECT id, doc_id, op_id, user_id, version, op_type, 
               position, length, content, timestamp, applied
        FROM operations
        WHERE doc_id = ?
        ORDER BY version ASC
    )";
    
    std::vector<std::string> params = {docId};
    std::vector<std::vector<std::string>> results;
    
    if (database.queryMultipleRows(sql, params, results)) {
        for (const auto& row : results) {
            operations.push_back(buildOperationFromRow(row));
        }
    }
    
    return operations;
}

// 获取文档指定版本范围的操作
std::vector<Operation> OperationDAO::getOperationsByVersion(const std::string& docId,
                                                           int startVersion,
                                                           int endVersion) {
    std::vector<Operation> operations;
    
    std::string sql = R"(
        SELECT id, doc_id, op_id, user_id, version, op_type, 
               position, length, content, timestamp, applied
        FROM operations
        WHERE doc_id = ? AND version >= ? AND version <= ?
        ORDER BY version ASC
    )";
    
    std::vector<std::string> params = {
        docId,
        std::to_string(startVersion),
        std::to_string(endVersion)
    };
    
    std::vector<std::vector<std::string>> results;
    
    if (database.queryMultipleRows(sql, params, results)) {
        for (const auto& row : results) {
            operations.push_back(buildOperationFromRow(row));
        }
    }
    
    return operations;
}

// 获取未应用的操作
std::vector<Operation> OperationDAO::getUnappliedOperations(const std::string& docId) {
    std::vector<Operation> operations;
    
    std::string sql = R"(
        SELECT id, doc_id, op_id, user_id, version, op_type, 
               position, length, content, timestamp, applied
        FROM operations
        WHERE doc_id = ? AND applied = 0
        ORDER BY version ASC
    )";
    
    std::vector<std::string> params = {docId};
    std::vector<std::vector<std::string>> results;
    
    if (database.queryMultipleRows(sql, params, results)) {
        for (const auto& row : results) {
            operations.push_back(buildOperationFromRow(row));
        }
    }
    
    return operations;
}

// 标记操作为已应用
bool OperationDAO::markOperationApplied(const std::string& opId) {
    std::string sql = "UPDATE operations SET applied = 1 WHERE op_id = ?";
    std::vector<std::string> params = {opId};
    return database.executeQuery(sql, params);
}

// 批量标记操作为已应用
bool OperationDAO::markOperationsApplied(const std::vector<std::string>& opIds) {
    if (opIds.empty()) {
        return true;
    }
    
    // 使用事务批量更新
    if (!database.beginTransaction()) {
        return false;
    }
    
    std::string sql = "UPDATE operations SET applied = 1 WHERE op_id = ?";
    
    for (const auto& opId : opIds) {
        std::vector<std::string> params = {opId};
        if (!database.executeQuery(sql, params)) {
            database.rollbackTransaction();
            return false;
        }
    }
    
    return database.commitTransaction();
}

// 删除操作记录
bool OperationDAO::deleteOperation(const std::string& opId) {
    std::string sql = "DELETE FROM operations WHERE op_id = ?";
    std::vector<std::string> params = {opId};
    return database.executeQuery(sql, params);
}

// 清理旧操作（保留最近的N条）
bool OperationDAO::cleanupOldOperations(const std::string& docId, int keepCount) {
    std::string sql = R"(
        DELETE FROM operations 
        WHERE doc_id = ? 
        AND id NOT IN (
            SELECT id FROM operations 
            WHERE doc_id = ? 
            ORDER BY version DESC 
            LIMIT ?
        )
    )";
    
    std::vector<std::string> params = {
        docId,
        docId,
        std::to_string(keepCount)
    };
    
    return database.executeQuery(sql, params);
}

// 获取文档的当前版本号
int OperationDAO::getCurrentVersion(const std::string& docId) {
    std::string sql = R"(
        SELECT COALESCE(MAX(version), 0) FROM operations WHERE doc_id = ?
    )";
    
    std::vector<std::string> params = {docId};
    std::vector<std::string> result;
    
    if (database.querySingleRow(sql, params, result) && !result.empty()) {
        return std::stoi(result[0]);
    }
    
    return 0;
}

// 检查操作ID是否存在
bool OperationDAO::exists(const std::string& opId) {
    std::string sql = "SELECT COUNT(*) FROM operations WHERE op_id = ?";
    std::vector<std::string> params = {opId};
    std::vector<std::string> result;
    
    if (database.querySingleRow(sql, params, result) && !result.empty()) {
        return std::stoi(result[0]) > 0;
    }
    
    return false;
}

// 获取操作数量
int OperationDAO::getOperationCount(const std::string& docId) {
    std::string sql = "SELECT COUNT(*) FROM operations WHERE doc_id = ?";
    std::vector<std::string> params = {docId};
    std::vector<std::string> result;
    
    if (database.querySingleRow(sql, params, result) && !result.empty()) {
        return std::stoi(result[0]);
    }
    
    return 0;
}

// 从查询结果行构建Operation对象
Operation OperationDAO::buildOperationFromRow(const std::vector<std::string>& row) {
    Operation op;
    if (row.size() >= 11) {
        op.id = std::stoll(row[0]);
        op.docId = row[1];
        op.opId = row[2];
        op.userId = row[3];
        op.version = std::stoi(row[4]);
        op.opType = row[5];
        op.position = std::stoi(row[6]);
        op.length = std::stoi(row[7]);
        op.content = row[8];
        op.timestamp = row[9];
        op.applied = (row[10] == "1");
    }
    return op;
}

} // namespace storage

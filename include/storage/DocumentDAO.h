#pragma once

#include <string>
#include <vector>
#include <memory>
#include "storage/Database.h"

namespace storage {

// 文档数据结构
struct Document {
    std::string docId;
    std::string title;
    std::string ownerId;
    std::string createdAt;
    std::string updatedAt;
    int currentVersion = 0;
    std::string content;
    bool isDeleted = false;
};

class DocumentDAO {
public:
    explicit DocumentDAO(Database& db);
    
    // 创建文档
    bool createDocument(const Document& doc);
    
    // 获取文档
    Document getDocument(const std::string& docId);
    
    // 更新文档
    bool updateDocument(const Document& doc);
    
    // 删除文档（软删除）
    bool deleteDocument(const std::string& docId);
    
    // 永久删除文档
    bool permanentlyDeleteDocument(const std::string& docId);
    
    // 获取用户的所有文档
    std::vector<Document> getUserDocuments(const std::string& userId);
    
    // 获取活跃文档列表
    std::vector<Document> getActiveDocuments(int limit = 50);
    
    // 更新文档内容
    bool updateDocumentContent(const std::string& docId, const std::string& content);
    
    // 更新文档版本
    bool updateDocumentVersion(const std::string& docId, int version);
    
    // 恢复已删除的文档
    bool restoreDocument(const std::string& docId);
    
    // 检查文档是否存在
    bool exists(const std::string& docId);
    
    // 检查用户是否是文档所有者
    bool isOwner(const std::string& docId, const std::string& userId);
    
    // 检查用户是否有访问权限
    bool hasAccess(const std::string& docId, const std::string& userId);
    
private:
    Database& database;
    
    // 从查询结果行构建Document对象
    Document buildDocumentFromRow(const std::vector<std::string>& row);
};

} // namespace storage

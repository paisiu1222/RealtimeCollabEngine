#include "storage/DocumentDAO.h"
#include <iostream>
#include <sstream>

namespace storage {

// 构造函数
DocumentDAO::DocumentDAO(Database& db) : database(db) {
}

// 创建文档
bool DocumentDAO::createDocument(const Document& doc) {
    std::string sql = R"(
        INSERT INTO documents (doc_id, title, owner_id, current_version, content, is_deleted)
        VALUES (?, ?, ?, ?, ?, ?)
    )";
    
    std::vector<std::string> params = {
        doc.docId,
        doc.title,
        doc.ownerId,
        std::to_string(doc.currentVersion),
        doc.content,
        doc.isDeleted ? "1" : "0"
    };
    
    return database.executeQuery(sql, params);
}

// 获取文档
Document DocumentDAO::getDocument(const std::string& docId) {
    Document doc;
    doc.docId = docId;
    
    std::string sql = R"(
        SELECT doc_id, title, owner_id, created_at, updated_at, 
               current_version, content, is_deleted
        FROM documents
        WHERE doc_id = ?
    )";
    
    std::vector<std::string> params = {docId};
    std::vector<std::string> result;
    
    if (database.querySingleRow(sql, params, result)) {
        doc = buildDocumentFromRow(result);
    }
    
    return doc;
}

// 更新文档
bool DocumentDAO::updateDocument(const Document& doc) {
    std::string sql = R"(
        UPDATE documents
        SET title = ?, current_version = ?, content = ?, is_deleted = ?
        WHERE doc_id = ?
    )";
    
    std::vector<std::string> params = {
        doc.title,
        std::to_string(doc.currentVersion),
        doc.content,
        doc.isDeleted ? "1" : "0",
        doc.docId
    };
    
    return database.executeQuery(sql, params);
}

// 删除文档（软删除）
bool DocumentDAO::deleteDocument(const std::string& docId) {
    std::string sql = "UPDATE documents SET is_deleted = 1 WHERE doc_id = ?";
    std::vector<std::string> params = {docId};
    return database.executeQuery(sql, params);
}

// 永久删除文档
bool DocumentDAO::permanentlyDeleteDocument(const std::string& docId) {
    std::string sql = "DELETE FROM documents WHERE doc_id = ?";
    std::vector<std::string> params = {docId};
    return database.executeQuery(sql, params);
}

// 获取用户的所有文档
std::vector<Document> DocumentDAO::getUserDocuments(const std::string& userId) {
    std::vector<Document> documents;
    
    std::string sql = R"(
        SELECT doc_id, title, owner_id, created_at, updated_at, 
               current_version, content, is_deleted
        FROM documents
        WHERE owner_id = ? AND is_deleted = 0
        ORDER BY updated_at DESC
    )";
    
    std::vector<std::string> params = {userId};
    std::vector<std::vector<std::string>> results;
    
    if (database.queryMultipleRows(sql, params, results)) {
        for (const auto& row : results) {
            documents.push_back(buildDocumentFromRow(row));
        }
    }
    
    return documents;
}

// 获取活跃文档列表
std::vector<Document> DocumentDAO::getActiveDocuments(int limit) {
    std::vector<Document> documents;
    
    std::string sql = R"(
        SELECT doc_id, title, owner_id, created_at, updated_at, 
               current_version, content, is_deleted
        FROM documents
        WHERE is_deleted = 0
        ORDER BY updated_at DESC
        LIMIT ?
    )";
    
    std::vector<std::string> params = {std::to_string(limit)};
    std::vector<std::vector<std::string>> results;
    
    if (database.queryMultipleRows(sql, params, results)) {
        for (const auto& row : results) {
            documents.push_back(buildDocumentFromRow(row));
        }
    }
    
    return documents;
}

// 更新文档内容
bool DocumentDAO::updateDocumentContent(const std::string& docId, const std::string& content) {
    std::string sql = "UPDATE documents SET content = ? WHERE doc_id = ?";
    std::vector<std::string> params = {content, docId};
    return database.executeQuery(sql, params);
}

// 更新文档版本
bool DocumentDAO::updateDocumentVersion(const std::string& docId, int version) {
    std::string sql = "UPDATE documents SET current_version = ? WHERE doc_id = ?";
    std::vector<std::string> params = {std::to_string(version), docId};
    return database.executeQuery(sql, params);
}

// 恢复已删除的文档
bool DocumentDAO::restoreDocument(const std::string& docId) {
    std::string sql = "UPDATE documents SET is_deleted = 0 WHERE doc_id = ?";
    std::vector<std::string> params = {docId};
    return database.executeQuery(sql, params);
}

// 检查文档是否存在
bool DocumentDAO::exists(const std::string& docId) {
    std::string sql = "SELECT COUNT(*) FROM documents WHERE doc_id = ?";
    std::vector<std::string> params = {docId};
    std::vector<std::string> result;
    
    if (database.querySingleRow(sql, params, result) && !result.empty()) {
        return std::stoi(result[0]) > 0;
    }
    
    return false;
}

// 检查用户是否是文档所有者
bool DocumentDAO::isOwner(const std::string& docId, const std::string& userId) {
    std::string sql = "SELECT COUNT(*) FROM documents WHERE doc_id = ? AND owner_id = ?";
    std::vector<std::string> params = {docId, userId};
    std::vector<std::string> result;
    
    if (database.querySingleRow(sql, params, result) && !result.empty()) {
        return std::stoi(result[0]) > 0;
    }
    
    return false;
}

// 检查用户是否有访问权限
bool DocumentDAO::hasAccess(const std::string& docId, const std::string& userId) {
    // 检查是否是所有者
    if (isOwner(docId, userId)) {
        return true;
    }
    
    // 检查是否是协作者
    std::string sql = R"(
        SELECT COUNT(*) FROM document_collaborators 
        WHERE doc_id = ? AND user_id = ?
    )";
    std::vector<std::string> params = {docId, userId};
    std::vector<std::string> result;
    
    if (database.querySingleRow(sql, params, result) && !result.empty()) {
        return std::stoi(result[0]) > 0;
    }
    
    return false;
}

// 从查询结果行构建Document对象
Document DocumentDAO::buildDocumentFromRow(const std::vector<std::string>& row) {
    Document doc;
    if (row.size() >= 8) {
        doc.docId = row[0];
        doc.title = row[1];
        doc.ownerId = row[2];
        doc.createdAt = row[3];
        doc.updatedAt = row[4];
        doc.currentVersion = std::stoi(row[5]);
        doc.content = row[6];
        doc.isDeleted = (row[7] == "1");
    }
    return doc;
}

} // namespace storage

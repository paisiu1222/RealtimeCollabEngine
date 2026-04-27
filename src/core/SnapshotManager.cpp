#include "core/SnapshotManager.h"
#include <sstream>

namespace core {

// ==================== 单例实现 ====================

SnapshotManager& SnapshotManager::getInstance() {
    static SnapshotManager instance;
    return instance;
}

SnapshotManager::SnapshotManager() : logger(utils::Logger::getInstance()) {
    logger.info("SnapshotManager initialized");
}

// ==================== 快照创建 ====================

bool SnapshotManager::createSnapshot(const std::string& docId, const DocumentState& state) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    try {
        auto& db = storage::Database::getInstance();
        
        // 插入快照记录
        std::string sql = "INSERT INTO snapshots (doc_id, version, content, created_at) VALUES (?, ?, ?, datetime('now'))";
        std::vector<std::string> params = {
            docId,
            std::to_string(state.getVersion()),
            state.getContent()
        };
        
        bool result = db.executeQuery(sql, params);
        
        if (result) {
            logger.info("Snapshot created for document: " + docId + 
                       ", version: " + std::to_string(state.getVersion()));
        } else {
            logger.error("Failed to create snapshot for document: " + docId);
        }
        
        return result;
        
    } catch (const std::exception& e) {
        logger.error("Failed to create snapshot for document: " + docId + 
                    ", error: " + e.what());
        return false;
    }
}

// ==================== 快照加载 ====================

SnapshotData SnapshotManager::loadSnapshotData(const std::string& docId) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    try {
        auto& db = storage::Database::getInstance();
        
        // 查询最新快照
        std::string sql = "SELECT version, content FROM snapshots WHERE doc_id = ? ORDER BY version DESC LIMIT 1";
        std::vector<std::string> params = {docId};
        std::vector<std::string> result;
        
        if (db.querySingleRow(sql, params, result) && result.size() >= 2) {
            uint64_t version = std::stoull(result[0]);
            std::string content = result[1];
            
            logger.info("Loaded snapshot for document: " + docId + 
                       ", version: " + std::to_string(version));
            
            return SnapshotData(docId, version, content);
        } else {
            logger.debug("No snapshot found for document: " + docId);
            return SnapshotData(docId, 0, "");
        }
        
    } catch (const std::exception& e) {
        logger.error("Failed to load snapshot for document: " + docId + 
                    ", error: " + e.what());
        return SnapshotData(docId, 0, "");
    }
}

// ==================== 快照策略检查 ====================

bool SnapshotManager::shouldCreateSnapshot(uint64_t currentVersion, uint64_t lastSnapshotVersion,
                                          uint64_t snapshotInterval) const {
    return (currentVersion - lastSnapshotVersion) >= snapshotInterval;
}

// ==================== 快照清理 ====================

int SnapshotManager::cleanupOldSnapshots(const std::string& docId, int keepCount) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    try {
        auto& db = storage::Database::getInstance();
        
        // 获取需要保留的最小版本号
        std::string selectSql = "SELECT version FROM snapshots WHERE doc_id = ? ORDER BY version DESC LIMIT 1 OFFSET ?";
        std::vector<std::string> selectParams = {docId, std::to_string(keepCount - 1)};
        std::vector<std::string> result;
        
        if (!db.querySingleRow(selectSql, selectParams, result) || result.empty()) {
            // 快照数量不足，无需清理
            logger.debug("Not enough snapshots to cleanup for document: " + docId);
            return 0;
        }
        
        uint64_t minKeepVersion = std::stoull(result[0]);
        
        // 删除旧快照
        std::string deleteSql = "DELETE FROM snapshots WHERE doc_id = ? AND version < ?";
        std::vector<std::string> deleteParams = {docId, std::to_string(minKeepVersion)};
        
        db.executeQuery(deleteSql, deleteParams);
        
        int deletedCount = db.getAffectedRows();
        
        logger.info("Cleaned up " + std::to_string(deletedCount) + 
                   " old snapshots for document: " + docId + 
                   ", keeping version >= " + std::to_string(minKeepVersion));
        
        return deletedCount;
        
    } catch (const std::exception& e) {
        logger.error("Failed to cleanup snapshots for document: " + docId + 
                    ", error: " + e.what());
        return 0;
    }
}

// ==================== 获取最新快照版本 ====================

uint64_t SnapshotManager::getLatestSnapshotVersion(const std::string& docId) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    try {
        auto& db = storage::Database::getInstance();
        
        std::string sql = "SELECT MAX(version) FROM snapshots WHERE doc_id = ?";
        std::vector<std::string> params = {docId};
        std::vector<std::string> result;
        
        if (db.querySingleRow(sql, params, result) && !result.empty()) {
            int64_t version = std::stoll(result[0]);
            return version > 0 ? static_cast<uint64_t>(version) : 0;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        logger.error("Failed to get latest snapshot version for document: " + docId + 
                    ", error: " + e.what());
        return 0;
    }
}

} // namespace core

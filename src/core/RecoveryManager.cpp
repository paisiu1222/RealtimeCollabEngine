#include "core/RecoveryManager.h"

namespace core {

// ==================== 单例实现 ====================

RecoveryManager& RecoveryManager::getInstance() {
    static RecoveryManager instance;
    return instance;
}

RecoveryManager::RecoveryManager() : logger(utils::Logger::getInstance()) {
    logger.info("RecoveryManager initialized");
}

// ==================== 文档恢复 ====================

std::shared_ptr<DocumentState> RecoveryManager::recoverDocument(const std::string& docId) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    logger.info("Starting document recovery for: " + docId);
    
    // 1. 从快照重建基础状态
    auto state = rebuildFromSnapshot(docId);
    
    if (state->getVersion() == 0) {
        logger.info("No snapshot found, returning empty state");
        return state;
    }
    
    uint64_t snapshotVersion = state->getVersion();
    logger.info("Loaded snapshot at version " + std::to_string(snapshotVersion));
    
    // 2. 获取快照后的所有操作并应用
    auto operations = getOperationsAfterVersion(docId, snapshotVersion);
    
    logger.info("Applying " + std::to_string(operations.size()) + " operations after snapshot");
    
    for (const auto& op : operations) {
        auto result = state->applyOperation(op);
        if (result != OperationResult::SUCCESS) {
            logger.warning("Failed to apply operation " + op.opId + 
                          " during recovery, result: " + std::to_string(static_cast<int>(result)));
            // 继续处理其他操作，不中断恢复过程
        }
    }
    
    logger.info("Document recovery completed for: " + docId + 
               ", final version: " + std::to_string(state->getVersion()));
    
    return state;
}

std::shared_ptr<DocumentState> RecoveryManager::recoverToVersion(const std::string& docId, uint64_t targetVersion) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    logger.info("Starting document recovery to version " + 
               std::to_string(targetVersion) + " for: " + docId);
    
    // 1. 找到目标版本之前的最新快照
    auto& snapshotManager = SnapshotManager::getInstance();
    uint64_t latestSnapshotVersion = snapshotManager.getLatestSnapshotVersion(docId);
    
    std::shared_ptr<DocumentState> state;
    uint64_t startVersion = 0;
    
    if (latestSnapshotVersion > 0 && latestSnapshotVersion <= targetVersion) {
        // 使用快照作为起点
        state = rebuildFromSnapshot(docId);
        startVersion = latestSnapshotVersion;
        logger.info("Using snapshot at version " + std::to_string(startVersion));
    } else {
        // 没有合适的快照，从头开始
        state = std::make_shared<DocumentState>(docId);
        startVersion = 0;
        logger.info("No suitable snapshot found, starting from empty state");
    }
    
    // 2. 获取从快照到目标版本的操作并应用
    auto operations = getOperationsAfterVersion(docId, startVersion, targetVersion);
    
    logger.info("Applying " + std::to_string(operations.size()) + " operations to reach version " + 
               std::to_string(targetVersion));
    
    for (const auto& op : operations) {
        if (op.version > targetVersion) {
            break; // 已达到目标版本
        }
        
        auto result = state->applyOperation(op);
        if (result != OperationResult::SUCCESS) {
            logger.warning("Failed to apply operation " + op.opId + 
                          " during versioned recovery, result: " + std::to_string(static_cast<int>(result)));
        }
    }
    
    logger.info("Document recovery to version " + std::to_string(targetVersion) + 
               " completed for: " + docId + 
               ", actual version: " + std::to_string(state->getVersion()));
    
    return state;
}

// ==================== 私有方法 ====================

std::shared_ptr<DocumentState> RecoveryManager::rebuildFromSnapshot(const std::string& docId) {
    auto& snapshotManager = SnapshotManager::getInstance();
    auto snapshotData = snapshotManager.loadSnapshotData(docId);
    
    auto state = std::make_shared<DocumentState>(snapshotData.docId);
    state->setContent(snapshotData.content);
    
    if (snapshotData.version == 0) {
        logger.info("No snapshot found for document: " + docId + 
                   ", starting from empty state");
    } else {
        logger.info("Rebuilt state from snapshot version: " + std::to_string(snapshotData.version));
        // 注意：DocumentState的版本号通过applyOperation自动管理
        // 这里无法直接设置版本号，需要通过后续操作来同步
    }
    
    return state;
}

std::vector<Operation> RecoveryManager::getOperationsAfterVersion(
    const std::string& docId, 
    uint64_t afterVersion,
    uint64_t untilVersion
) {
    std::vector<Operation> operations;
    
    try {
        auto& db = storage::Database::getInstance();
        
        std::string sql;
        std::vector<std::string> params;
        
        if (untilVersion > 0) {
            sql = "SELECT op_id, user_id, version, op_type, position, content, timestamp "
                  "FROM operations WHERE doc_id = ? AND version > ? AND version <= ? "
                  "ORDER BY version ASC";
            params = {docId, std::to_string(afterVersion), std::to_string(untilVersion)};
        } else {
            sql = "SELECT op_id, user_id, version, op_type, position, content, timestamp "
                  "FROM operations WHERE doc_id = ? AND version > ? ORDER BY version ASC";
            params = {docId, std::to_string(afterVersion)};
        }
        
        std::vector<std::vector<std::string>> results;
        if (db.queryMultipleRows(sql, params, results)) {
            for (const auto& row : results) {
                if (row.size() >= 7) {
                    Operation op;
                    op.opId = row[0];
                    op.userId = row[1];
                    op.version = std::stoull(row[2]);
                    
                    // 解析操作类型
                    if (row[3] == "INSERT") {
                        op.type = OperationType::INSERT;
                    } else if (row[3] == "DELETE") {
                        op.type = OperationType::DELETE;
                    } else if (row[3] == "RETAIN") {
                        op.type = OperationType::RETAIN;
                    }
                    
                    op.position = std::stoul(row[4]);
                    op.content = row[5];
                    // timestamp字段暂时不设置，使用默认值
                    
                    operations.push_back(op);
                }
            }
        }
        
    } catch (const std::exception& e) {
        logger.error("Failed to get operations after version " + 
                    std::to_string(afterVersion) + " for document: " + docId + 
                    ", error: " + e.what());
    }
    
    return operations;
}

} // namespace core

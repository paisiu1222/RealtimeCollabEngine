#include "core/SyncManager.h"

namespace core {

// ==================== 单例实现 ====================

SyncManager& SyncManager::getInstance() {
    static SyncManager instance;
    return instance;
}

SyncManager::SyncManager() : logger(utils::Logger::getInstance()) {
    logger.info("SyncManager initialized");
}

// ==================== 重连处理 ====================

void SyncManager::handleReconnect(
    const std::string& userId, 
    const SyncCallback& syncCallback
) {
    logger.info("Handling reconnect for user: " + userId);
    
    // 同步待处理的操作
    size_t syncedCount = syncPendingOperations(userId, syncCallback);
    
    if (syncedCount > 0) {
        logger.info("Synced " + std::to_string(syncedCount) + 
                   " pending operations for user: " + userId);
    } else {
        logger.info("No pending operations for user: " + userId);
    }
}

size_t SyncManager::syncPendingOperations(
    const std::string& userId, 
    const SyncCallback& syncCallback
) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = queues_.find(userId);
    if (it == queues_.end()) {
        logger.debug("No offline queue for user: " + userId);
        return 0;
    }
    
    auto& queue = it->second;
    auto ops = queue->dequeueAll();
    
    size_t count = 0;
    for (const auto& op : ops) {
        if (syncCallback) {
            syncCallback(userId, op);
            count++;
        }
    }
    
    logger.info("Synced " + std::to_string(count) + " operations for user: " + userId);
    
    return count;
}

// ==================== 离线操作管理 ====================

void SyncManager::enqueueOfflineOperation(
    const std::string& userId, 
    const Operation& op
) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    // 获取或创建离线队列
    auto it = queues_.find(userId);
    if (it == queues_.end()) {
        queues_[userId] = std::make_unique<OfflineQueue>(userId);
        it = queues_.find(userId);
    }
    
    it->second->enqueue(op);
    
    logger.debug("Enqueued offline operation for user: " + userId + 
                ", queue size: " + std::to_string(it->second->size()));
}

bool SyncManager::hasPendingOperations(const std::string& userId) const {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = queues_.find(userId);
    if (it == queues_.end()) {
        return false;
    }
    
    return !it->second->isEmpty();
}

size_t SyncManager::getPendingOperationCount(const std::string& userId) const {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = queues_.find(userId);
    if (it == queues_.end()) {
        return 0;
    }
    
    return it->second->size();
}

void SyncManager::clearUserQueue(const std::string& userId) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = queues_.find(userId);
    if (it != queues_.end()) {
        it->second->clear();
        logger.info("Cleared offline queue for user: " + userId);
    }
}

} // namespace core

#ifndef SYNC_MANAGER_H
#define SYNC_MANAGER_H

#include "core/OfflineQueue.h"
#include "core/Operation.h"
#include <map>
#include <memory>
#include <mutex>
#include <functional>
#include "utils/Logger.h"

namespace core {

/**
 * 同步管理器
 * 管理离线编辑的同步逻辑
 */
class SyncManager {
public:
    /**
     * 同步回调函数类型
     */
    using SyncCallback = std::function<void(const std::string& userId, const Operation& op)>;
    
    /**
     * 获取单例实例
     */
    static SyncManager& getInstance();
    
    /**
     * 处理用户重连
     * @param userId 用户ID
     * @param syncCallback 同步回调函数
     */
    void handleReconnect(const std::string& userId, const SyncCallback& syncCallback);
    
    /**
     * 同步用户的待处理操作
     * @param userId 用户ID
     * @param syncCallback 同步回调函数
     * @return 同步的操作数量
     */
    size_t syncPendingOperations(const std::string& userId, const SyncCallback& syncCallback);
    
    /**
     * 将操作加入离线队列（用户离线时调用）
     * @param userId 用户ID
     * @param op 操作
     */
    void enqueueOfflineOperation(const std::string& userId, const Operation& op);
    
    /**
     * 检查用户是否有待同步的操作
     * @param userId 用户ID
     * @return 是否有待同步操作
     */
    bool hasPendingOperations(const std::string& userId) const;
    
    /**
     * 获取用户待同步操作数量
     * @param userId 用户ID
     * @return 操作数量
     */
    size_t getPendingOperationCount(const std::string& userId) const;
    
    /**
     * 清除用户的离线队列
     * @param userId 用户ID
     */
    void clearUserQueue(const std::string& userId);

private:
    SyncManager();
    ~SyncManager() = default;
    
    // 禁止拷贝和赋值
    SyncManager(const SyncManager&) = delete;
    SyncManager& operator=(const SyncManager&) = delete;
    
    // ==================== 成员变量 ====================
    
    mutable std::mutex managerMutex;                              // 管理器互斥锁
    std::map<std::string, std::unique_ptr<OfflineQueue>> queues_; // userId -> OfflineQueue映射
    
    utils::Logger& logger;                                        // 日志记录器
};

} // namespace core

#endif // SYNC_MANAGER_H

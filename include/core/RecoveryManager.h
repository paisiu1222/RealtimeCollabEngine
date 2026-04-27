#ifndef RECOVERY_MANAGER_H
#define RECOVERY_MANAGER_H

#include "core/DocumentState.h"
#include "core/Operation.h"
#include "core/SnapshotManager.h"
#include "storage/Database.h"
#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include "utils/Logger.h"

namespace core {

/**
 * 数据恢复管理器
 * 负责从快照和增量操作重建文档状态
 */
class RecoveryManager {
public:
    /**
     * 获取单例实例
     */
    static RecoveryManager& getInstance();
    
    /**
     * 恢复文档到最新状态
     * @param docId 文档ID
     * @return 恢复后的文档状态指针
     */
    std::shared_ptr<DocumentState> recoverDocument(const std::string& docId);
    
    /**
     * 恢复文档到指定版本
     * @param docId 文档ID
     * @param targetVersion 目标版本
     * @return 恢复后的文档状态指针
     */
    std::shared_ptr<DocumentState> recoverToVersion(const std::string& docId, uint64_t targetVersion);

private:
    RecoveryManager();
    ~RecoveryManager() = default;
    
    // 禁止拷贝和赋值
    RecoveryManager(const RecoveryManager&) = delete;
    RecoveryManager& operator=(const RecoveryManager&) = delete;
    
    /**
     * 从快照重建文档状态
     * @param docId 文档ID
     * @return 文档状态指针
     */
    std::shared_ptr<DocumentState> rebuildFromSnapshot(const std::string& docId);
    
    /**
     * 获取指定版本后的所有操作
     * @param docId 文档ID
     * @param afterVersion 起始版本（不包含）
     * @param untilVersion 结束版本（包含），0表示到最新
     * @return 操作列表
     */
    std::vector<Operation> getOperationsAfterVersion(const std::string& docId, 
                                                     uint64_t afterVersion,
                                                     uint64_t untilVersion = 0);
    
    // ==================== 成员变量 ====================
    
    mutable std::mutex managerMutex;    // 管理器互斥锁
    utils::Logger& logger;              // 日志记录器
};

} // namespace core

#endif // RECOVERY_MANAGER_H

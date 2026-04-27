#ifndef SNAPSHOT_MANAGER_H
#define SNAPSHOT_MANAGER_H

#include "core/DocumentState.h"
#include "storage/Database.h"
#include <string>
#include <mutex>
#include <memory>
#include "utils/Logger.h"

namespace core {

/**
 * 快照数据结构
 */
struct SnapshotData {
    std::string docId;
    uint64_t version;
    std::string content;
    
    SnapshotData() : version(0) {}
    SnapshotData(const std::string& id, uint64_t ver, const std::string& cont)
        : docId(id), version(ver), content(cont) {}
};

/**
 * 快照管理器
 * 负责文档状态的定期快照生成、存储和加载
 */
class SnapshotManager {
public:
    /**
     * 获取单例实例
     */
    static SnapshotManager& getInstance();
    
    /**
     * 创建文档快照
     * @param docId 文档ID
     * @param state 文档状态
     * @return 是否成功创建
     */
    bool createSnapshot(const std::string& docId, const DocumentState& state);
    
    /**
     * 加载文档的最新快照数据
     * @param docId 文档ID
     * @return 快照数据，如果不存在返回空数据（version=0）
     */
    SnapshotData loadSnapshotData(const std::string& docId);
    
    /**
     * 检查是否需要创建快照
     * @param currentVersion 当前版本
     * @param lastSnapshotVersion 上次快照版本
     * @param snapshotInterval 快照间隔（默认100个操作）
     * @return 是否需要创建
     */
    bool shouldCreateSnapshot(uint64_t currentVersion, uint64_t lastSnapshotVersion, 
                             uint64_t snapshotInterval = 100) const;
    
    /**
     * 清理旧快照，保留最近的N个
     * @param docId 文档ID
     * @param keepCount 保留数量
     * @return 删除的快照数量
     */
    int cleanupOldSnapshots(const std::string& docId, int keepCount = 5);
    
    /**
     * 获取文档的最新快照版本
     * @param docId 文档ID
     * @return 快照版本号，如果没有快照返回0
     */
    uint64_t getLatestSnapshotVersion(const std::string& docId);

private:
    SnapshotManager();
    ~SnapshotManager() = default;
    
    // 禁止拷贝和赋值
    SnapshotManager(const SnapshotManager&) = delete;
    SnapshotManager& operator=(const SnapshotManager&) = delete;
    
    // ==================== 成员变量 ====================
    
    mutable std::mutex managerMutex;    // 管理器互斥锁
    utils::Logger& logger;              // 日志记录器
};

} // namespace core

#endif // SNAPSHOT_MANAGER_H

#ifndef DOCUMENT_STATE_H
#define DOCUMENT_STATE_H

#include <string>
#include <deque>
#include <mutex>
#include <vector>
#include "core/Operation.h"
#include "utils/Logger.h"

namespace core {

/**
 * 文档状态类
 * 管理文档的当前内容、版本历史和并发控制
 */
class DocumentState {
public:
    /**
     * 构造函数
     * @param docId 文档ID
     */
    DocumentState(const std::string& docId);
    
    /**
     * 应用操作到文档
     * @param op 要应用的操作
     * @return 操作结果
     */
    OperationResult applyOperation(const Operation& op);
    
    /**
     * 获取文档当前内容
     */
    std::string getContent() const;
    
    /**
     * 设置文档内容（初始化用）
     */
    void setContent(const std::string& content);

    /**
     * 设置文档版本号（恢复用）
     */
    void setVersion(uint64_t version);
    
    /**
     * 获取当前版本号
     */
    uint64_t getVersion() const;
    
    /**
     * 获取操作历史
     * @param count 获取最近的操作数量，0表示全部
     */
    std::vector<Operation> getHistory(int count = 0) const;
    
    /**
     * 回滚到指定版本
     * @param targetVersion 目标版本号
     * @return 是否成功
     */
    bool rollbackToVersion(uint64_t targetVersion);
    
    /**
     * 获取文档ID
     */
    std::string getDocId() const { return docId; }
    
    /**
     * 检查是否可以应用某个版本的操作
     */
    bool canApplyVersion(uint64_t version) const;
    
private:
    /**
     * 执行插入操作
     */
    bool executeInsert(const Operation& op);
    
    /**
     * 执行删除操作
     */
    bool executeDelete(const Operation& op);
    
    /**
     * 执行替换操作
     */
    bool executeReplace(const Operation& op);
    
    /**
     * 验证操作位置是否合法（根据操作类型检查）
     */
    bool validatePosition(const Operation& op) const;
    
    // ==================== 成员变量 ====================
    
    std::string docId;                    // 文档ID
    std::string content;                  // 文档内容
    uint64_t currentVersion;              // 当前版本号
    std::deque<Operation> history;        // 操作历史记录
    
    mutable std::mutex stateMutex;        // 状态互斥锁
    static constexpr size_t MAX_HISTORY_SIZE = 1000; // 最大历史记录数
    
    utils::Logger& logger;                // 日志记录器
};

} // namespace core

#endif // DOCUMENT_STATE_H

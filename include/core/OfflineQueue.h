#ifndef OFFLINE_QUEUE_H
#define OFFLINE_QUEUE_H

#include "core/Operation.h"
#include <queue>
#include <vector>
#include <mutex>
#include <string>

namespace core {

/**
 * 离线操作队列
 * 缓存用户离线期间的操作，等待重连后同步
 */
class OfflineQueue {
public:
    /**
     * 构造函数
     * @param userId 用户ID
     */
    explicit OfflineQueue(const std::string& userId);
    
    /**
     * 将操作加入队列
     * @param op 要缓存的操作
     */
    void enqueue(const Operation& op);
    
    /**
     * 取出所有待同步的操作
     * @return 操作列表
     */
    std::vector<Operation> dequeueAll();
    
    /**
     * 检查队列是否为空
     */
    bool isEmpty() const;
    
    /**
     * 获取队列大小
     */
    size_t size() const;
    
    /**
     * 清空队列
     */
    void clear();
    
    /**
     * 获取用户ID
     */
    std::string getUserId() const { return userId; }

private:
    std::string userId;                    // 用户ID
    std::queue<Operation> pendingOps;      // 待同步操作队列
    mutable std::mutex queueMutex;         // 队列互斥锁
};

} // namespace core

#endif // OFFLINE_QUEUE_H

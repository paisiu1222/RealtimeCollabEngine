#include "core/OfflineQueue.h"

namespace core {

// ==================== 构造函数 ====================

OfflineQueue::OfflineQueue(const std::string& userId) : userId(userId) {
}

// ==================== 队列操作 ====================

void OfflineQueue::enqueue(const Operation& op) {
    std::lock_guard<std::mutex> lock(queueMutex);
    pendingOps.push(op);
}

std::vector<Operation> OfflineQueue::dequeueAll() {
    std::lock_guard<std::mutex> lock(queueMutex);
    
    std::vector<Operation> ops;
    while (!pendingOps.empty()) {
        ops.push_back(pendingOps.front());
        pendingOps.pop();
    }
    
    return ops;
}

bool OfflineQueue::isEmpty() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return pendingOps.empty();
}

size_t OfflineQueue::size() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return pendingOps.size();
}

void OfflineQueue::clear() {
    std::lock_guard<std::mutex> lock(queueMutex);
    
    // 清空队列
    std::queue<Operation> empty;
    std::swap(pendingOps, empty);
}

} // namespace core

#include "test_framework.h"
#include "core/ConflictResolver.h"
#include "core/OfflineQueue.h"
#include "core/SyncManager.h"
#include "core/OTAlgorithm.h"

using namespace core;

// ==================== ConflictResolver Tests ====================

// 测试1: 无冲突检测（同一用户）
TEST(ConflictResolverTest, NoConflict_SameUser) {
    Operation op1("op1", "user_A", 1, OperationType::INSERT, 5, "Hello");
    Operation op2("op2", "user_A", 1, OperationType::DELETE, 0, "World");
    
    auto conflict = ConflictResolver::detectConflict(op1, op2);
    ASSERT_EQ(conflict, ConflictResolver::ConflictType::NONE);
}

// 测试2: 版本冲突检测
TEST(ConflictResolverTest, VersionConflict) {
    Operation op1("op1", "user_A", 1, OperationType::INSERT, 5, "Hello");
    Operation op2("op2", "user_B", 2, OperationType::DELETE, 0, "World");
    
    auto conflict = ConflictResolver::detectConflict(op1, op2);
    ASSERT_EQ(conflict, ConflictResolver::ConflictType::VERSION_CONFLICT);
}

// 测试3: 位置冲突检测（重叠区域）
TEST(ConflictResolverTest, PositionConflict_Overlapping) {
    Operation op1("op1", "user_A", 1, OperationType::DELETE, 5, "Hello");
    Operation op2("op2", "user_B", 1, OperationType::DELETE, 7, "World");
    
    auto conflict = ConflictResolver::detectConflict(op1, op2);
    ASSERT_EQ(conflict, ConflictResolver::ConflictType::POSITION_CONFLICT);
}

// 测试4: 无位置冲突（不重叠）
TEST(ConflictResolverTest, NoPositionConflict_NonOverlapping) {
    Operation op1("op1", "user_A", 1, OperationType::DELETE, 0, "Hello");
    Operation op2("op2", "user_B", 1, OperationType::DELETE, 10, "World");
    
    auto conflict = ConflictResolver::detectConflict(op1, op2);
    ASSERT_EQ(conflict, ConflictResolver::ConflictType::NONE);
}

// 测试5: INSERT操作位置冲突
TEST(ConflictResolverTest, InsertPositionConflict) {
    Operation op1("op1", "user_A", 1, OperationType::INSERT, 5, "X");
    Operation op2("op2", "user_B", 1, OperationType::INSERT, 5, "Y");
    
    auto conflict = ConflictResolver::detectConflict(op1, op2);
    // INSERT在同一位置视为冲突
    ASSERT_EQ(conflict, ConflictResolver::ConflictType::POSITION_CONFLICT);
}

// 测试6: 冲突类型转字符串
TEST(ConflictResolverTest, ConflictTypeToString) {
    ASSERT_STREQ("NONE", ConflictResolver::conflictTypeToString(ConflictResolver::ConflictType::NONE).c_str());
    ASSERT_STREQ("VERSION_CONFLICT", ConflictResolver::conflictTypeToString(ConflictResolver::ConflictType::VERSION_CONFLICT).c_str());
    ASSERT_STREQ("POSITION_CONFLICT", ConflictResolver::conflictTypeToString(ConflictResolver::ConflictType::POSITION_CONFLICT).c_str());
}

// 测试7: 冲突解决（时间戳优先）
TEST(ConflictResolverTest, ResolveConflict_TimestampPriority) {
    Operation op1("op1", "user_A", 1, OperationType::INSERT, 5, "Hello");
    
    Operation op2("op2", "user_B", 1, OperationType::INSERT, 5, "World");
    
    // 验证返回的是转换后的操作
    auto resolved = ConflictResolver::resolveConflict(op1, op2);
    
    // 验证返回的操作ID不为空
    ASSERT_NOT_EMPTY(resolved.opId.c_str());
}

// ==================== OfflineQueue Tests ====================

// 测试8: 离线队列创建
TEST(OfflineQueueTest, QueueCreation) {
    OfflineQueue queue("user_001");
    ASSERT_STREQ("user_001", queue.getUserId().c_str());
    ASSERT_TRUE(queue.isEmpty());
    ASSERT_EQ(queue.size(), 0);
}

// 测试9: 入队操作
TEST(OfflineQueueTest, EnqueueOperation) {
    OfflineQueue queue("user_001");
    
    Operation op("op1", "user_001", 1, OperationType::INSERT, 0, "Hello");
    queue.enqueue(op);
    
    ASSERT_FALSE(queue.isEmpty());
    ASSERT_EQ(queue.size(), 1);
}

// 测试10: 出队所有操作
TEST(OfflineQueueTest, DequeueAllOperations) {
    OfflineQueue queue("user_001");
    
    Operation op1("op1", "user_001", 1, OperationType::INSERT, 0, "Hello");
    Operation op2("op2", "user_001", 2, OperationType::INSERT, 5, " World");
    
    queue.enqueue(op1);
    queue.enqueue(op2);
    
    auto ops = queue.dequeueAll();
    
    ASSERT_EQ(ops.size(), 2);
    ASSERT_TRUE(queue.isEmpty());
}

// 测试11: 清空队列
TEST(OfflineQueueTest, ClearQueue) {
    OfflineQueue queue("user_001");
    
    Operation op("op1", "user_001", 1, OperationType::INSERT, 0, "Hello");
    queue.enqueue(op);
    queue.enqueue(op);
    
    ASSERT_EQ(queue.size(), 2);
    
    queue.clear();
    
    ASSERT_TRUE(queue.isEmpty());
    ASSERT_EQ(queue.size(), 0);
}

// 测试12: 多次入队出队
TEST(OfflineQueueTest, MultipleEnqueueDequeue) {
    OfflineQueue queue("user_001");
    
    for (int i = 0; i < 10; ++i) {
        Operation op("op" + std::to_string(i), "user_001", i+1, 
                    OperationType::INSERT, 0, "Text");
        queue.enqueue(op);
    }
    
    ASSERT_EQ(queue.size(), 10);
    
    auto ops = queue.dequeueAll();
    ASSERT_EQ(ops.size(), 10);
    ASSERT_TRUE(queue.isEmpty());
}

// ==================== SyncManager Tests ====================

// 测试13: SyncManager单例
TEST(SyncManagerTest, SingletonInstance) {
    auto& manager1 = SyncManager::getInstance();
    auto& manager2 = SyncManager::getInstance();
    
    ASSERT_EQ(&manager1, &manager2);
}

// 测试14: 入队离线操作
TEST(SyncManagerTest, EnqueueOfflineOperation) {
    auto& manager = SyncManager::getInstance();
    
    Operation op("op1", "user_001", 1, OperationType::INSERT, 0, "Hello");
    manager.enqueueOfflineOperation("user_001", op);
    
    ASSERT_TRUE(manager.hasPendingOperations("user_001"));
    ASSERT_EQ(manager.getPendingOperationCount("user_001"), 1);
}

// 测试15: 检查待同步操作
TEST(SyncManagerTest, HasPendingOperations) {
    auto& manager = SyncManager::getInstance();
    
    // 新用户没有待同步操作
    ASSERT_FALSE(manager.hasPendingOperations("user_new"));
    
    // 添加操作后应该有
    Operation op("op1", "user_test", 1, OperationType::INSERT, 0, "Hello");
    manager.enqueueOfflineOperation("user_test", op);
    
    ASSERT_TRUE(manager.hasPendingOperations("user_test"));
}

// 测试16: 获取待同步操作数量
TEST(SyncManagerTest, GetPendingOperationCount) {
    auto& manager = SyncManager::getInstance();
    
    // 初始为0
    ASSERT_EQ(manager.getPendingOperationCount("user_count"), 0);
    
    // 添加3个操作
    for (int i = 0; i < 3; ++i) {
        Operation op("op" + std::to_string(i), "user_count", i+1, 
                    OperationType::INSERT, 0, "Text");
        manager.enqueueOfflineOperation("user_count", op);
    }
    
    ASSERT_EQ(manager.getPendingOperationCount("user_count"), 3);
}

// 测试17: 清除用户队列
TEST(SyncManagerTest, ClearUserQueue) {
    auto& manager = SyncManager::getInstance();
    
    // 添加操作
    Operation op("op1", "user_clear", 1, OperationType::INSERT, 0, "Hello");
    manager.enqueueOfflineOperation("user_clear", op);
    
    ASSERT_EQ(manager.getPendingOperationCount("user_clear"), 1);
    
    // 清除队列
    manager.clearUserQueue("user_clear");
    
    ASSERT_EQ(manager.getPendingOperationCount("user_clear"), 0);
}

// 测试18: 重连处理与同步
TEST(SyncManagerTest, HandleReconnect) {
    auto& manager = SyncManager::getInstance();
    
    // 添加离线操作
    Operation op1("op1", "user_reconnect", 1, OperationType::INSERT, 0, "Hello");
    Operation op2("op2", "user_reconnect", 2, OperationType::INSERT, 5, " World");
    
    manager.enqueueOfflineOperation("user_reconnect", op1);
    manager.enqueueOfflineOperation("user_reconnect", op2);
    
    // 模拟重连并同步
    int syncedCount = 0;
    auto callback = [&syncedCount](const std::string& /*userId*/, const Operation& /*op*/) {
        syncedCount++;
    };
    
    manager.handleReconnect("user_reconnect", callback);
    
    ASSERT_EQ(syncedCount, 2);
    ASSERT_FALSE(manager.hasPendingOperations("user_reconnect"));
}

// 测试19: 同步待处理操作
TEST(SyncManagerTest, SyncPendingOperations) {
    auto& manager = SyncManager::getInstance();
    
    // 添加操作
    for (int i = 0; i < 5; ++i) {
        Operation op("op" + std::to_string(i), "user_sync", i+1, 
                    OperationType::INSERT, 0, "Text");
        manager.enqueueOfflineOperation("user_sync", op);
    }
    
    std::vector<Operation> syncedOps;
    auto callback = [&syncedOps](const std::string& /*userId*/, const Operation& op) {
        syncedOps.push_back(op);
    };
    
    size_t count = manager.syncPendingOperations("user_sync", callback);
    
    ASSERT_EQ(count, 5);
    ASSERT_EQ(syncedOps.size(), 5);
}

// 测试20: 多用户离线队列隔离
TEST(SyncManagerTest, MultiUserQueueIsolation) {
    auto& manager = SyncManager::getInstance();
    
    // 用户A添加操作
    Operation opA("opA", "user_A", 1, OperationType::INSERT, 0, "A");
    manager.enqueueOfflineOperation("user_A", opA);
    
    // 用户B添加操作
    Operation opB("opB", "user_B", 1, OperationType::INSERT, 0, "B");
    manager.enqueueOfflineOperation("user_B", opB);
    
    // 验证队列隔离
    ASSERT_EQ(manager.getPendingOperationCount("user_A"), 1);
    ASSERT_EQ(manager.getPendingOperationCount("user_B"), 1);
    
    // 清除用户A的队列不应影响用户B
    manager.clearUserQueue("user_A");
    
    ASSERT_EQ(manager.getPendingOperationCount("user_A"), 0);
    ASSERT_EQ(manager.getPendingOperationCount("user_B"), 1);
}

// 测试21: 空队列同步
TEST(SyncManagerTest, SyncEmptyQueue) {
    auto& manager = SyncManager::getInstance();
    
    int callbackCalled = 0;
    auto callback = [&callbackCalled](const std::string& /*userId*/, const Operation& /*op*/) {
        callbackCalled++;
    };
    
    // 同步空队列
    size_t count = manager.syncPendingOperations("user_empty", callback);
    
    ASSERT_EQ(count, 0);
    ASSERT_EQ(callbackCalled, 0);
}

// 测试22: 大量离线操作
TEST(SyncManagerTest, LargeNumberOfOfflineOps) {
    auto& manager = SyncManager::getInstance();
    
    // 添加100个离线操作
    for (int i = 0; i < 100; ++i) {
        Operation op("op" + std::to_string(i), "user_large", i+1, 
                    OperationType::INSERT, 0, "Text");
        manager.enqueueOfflineOperation("user_large", op);
    }
    
    ASSERT_EQ(manager.getPendingOperationCount("user_large"), 100);
    
    // 同步所有操作
    int syncedCount = 0;
    auto callback = [&syncedCount](const std::string& /*userId*/, const Operation& /*op*/) {
        syncedCount++;
    };
    
    manager.handleReconnect("user_large", callback);
    
    ASSERT_EQ(syncedCount, 100);
    ASSERT_EQ(manager.getPendingOperationCount("user_large"), 0);
}

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Running Advanced Features Tests" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    test::TestRunner::getInstance().runAll();
    return 0;
}

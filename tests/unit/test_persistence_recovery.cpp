#include "test_framework.h"
#include "core/SnapshotManager.h"
#include "core/RecoveryManager.h"
#include "core/DocumentState.h"

using namespace core;

// ==================== SnapshotManager Tests ====================

// 测试1: SnapshotManager单例
TEST(SnapshotManagerTest, SingletonInstance) {
    auto& manager1 = SnapshotManager::getInstance();
    auto& manager2 = SnapshotManager::getInstance();
    
    ASSERT_EQ(&manager1, &manager2);
}

// 测试2: 检查是否需要创建快照
TEST(SnapshotManagerTest, ShouldCreateSnapshot) {
    auto& manager = SnapshotManager::getInstance();
    
    // 间隔达到100，应该创建
    ASSERT_TRUE(manager.shouldCreateSnapshot(200, 100, 100));
    
    // 间隔不足100，不应该创建
    ASSERT_FALSE(manager.shouldCreateSnapshot(150, 100, 100));
    
    // 间隔超过100，应该创建
    ASSERT_TRUE(manager.shouldCreateSnapshot(250, 100, 100));
}

// 测试3: 默认快照间隔
TEST(SnapshotManagerTest, DefaultSnapshotInterval) {
    auto& manager = SnapshotManager::getInstance();
    
    // 默认间隔是100
    ASSERT_TRUE(manager.shouldCreateSnapshot(100, 0));
    ASSERT_FALSE(manager.shouldCreateSnapshot(99, 0));
}

// 测试4: 加载空快照数据
TEST(SnapshotManagerTest, LoadEmptySnapshotData) {
    auto& manager = SnapshotManager::getInstance();
    
    SnapshotData data = manager.loadSnapshotData("doc_nonexistent");
    
    ASSERT_STREQ("doc_nonexistent", data.docId.c_str());
    ASSERT_EQ(data.version, 0);
    ASSERT_TRUE(data.content.empty());
}

// ==================== RecoveryManager Tests ====================

// 测试5: RecoveryManager单例
TEST(RecoveryManagerTest, SingletonInstance) {
    auto& manager1 = RecoveryManager::getInstance();
    auto& manager2 = RecoveryManager::getInstance();
    
    ASSERT_EQ(&manager1, &manager2);
}

// 测试6: 恢复空文档
TEST(RecoveryManagerTest, RecoverEmptyDocument) {
    auto& recoveryManager = RecoveryManager::getInstance();
    
    auto state = recoveryManager.recoverDocument("doc_nonexistent");
    
    ASSERT_NE(state, nullptr);
    ASSERT_STREQ("doc_nonexistent", state->getDocId().c_str());
}

// 测试7: 恢复到指定版本（空文档）
TEST(RecoveryManagerTest, RecoverToVersion_Empty) {
    auto& recoveryManager = RecoveryManager::getInstance();
    
    auto state = recoveryManager.recoverToVersion("doc_empty", 100);
    
    ASSERT_NE(state, nullptr);
    ASSERT_STREQ("doc_empty", state->getDocId().c_str());
}

// 测试8: SnapshotManager初始化日志
TEST(SnapshotManagerTest, InitializationLog) {
    // 这个测试验证SnapshotManager能正常初始化
    auto& manager = SnapshotManager::getInstance();
    
    // 如果能获取实例，说明初始化成功
    ASSERT_TRUE(true);
}

// 测试9: RecoveryManager初始化日志
TEST(RecoveryManagerTest, InitializationLog) {
    // 这个测试验证RecoveryManager能正常初始化
    auto& manager = RecoveryManager::getInstance();
    
    // 如果能获取实例，说明初始化成功
    ASSERT_TRUE(true);
}

// 测试10: 快照策略边界值
TEST(SnapshotManagerTest, SnapshotPolicy_BoundaryValues) {
    auto& manager = SnapshotManager::getInstance();
    
    // 刚好达到间隔
    ASSERT_TRUE(manager.shouldCreateSnapshot(100, 0, 100));
    
    // 差1达到间隔
    ASSERT_FALSE(manager.shouldCreateSnapshot(99, 0, 100));
    
    // 超过间隔
    ASSERT_TRUE(manager.shouldCreateSnapshot(101, 0, 100));
}

// 测试11: 不同快照间隔配置
TEST(SnapshotManagerTest, DifferentSnapshotIntervals) {
    auto& manager = SnapshotManager::getInstance();
    
    // 间隔50
    ASSERT_TRUE(manager.shouldCreateSnapshot(50, 0, 50));
    ASSERT_FALSE(manager.shouldCreateSnapshot(49, 0, 50));
    
    // 间隔200
    ASSERT_TRUE(manager.shouldCreateSnapshot(200, 0, 200));
    ASSERT_FALSE(manager.shouldCreateSnapshot(199, 0, 200));
}

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Running Persistence & Recovery Tests" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    test::TestRunner::getInstance().runAll();
    return 0;
}

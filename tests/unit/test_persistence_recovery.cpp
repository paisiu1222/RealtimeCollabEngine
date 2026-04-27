#include "test_framework.h"
#include "core/SnapshotManager.h"
#include "core/RecoveryManager.h"
#include "core/DocumentState.h"
#include "storage/Database.h"
#include <fstream>
#include <sys/stat.h>

using namespace core;
using namespace storage;

// 测试数据库文件路径
const std::string TEST_DB_PATH = "/tmp/test_persistence_recovery.db";

// 初始化测试数据库
void initTestDatabase() {
    // 删除旧数据库文件
    remove(TEST_DB_PATH.c_str());
    
    auto& db = Database::getInstance();
    DatabaseConfig config;
    config.dbPath = TEST_DB_PATH;
    config.poolSize = 1;
    
    ASSERT_TRUE(db.initialize(config));
}

// 清理测试数据库
void cleanupTestDatabase() {
    auto& db = Database::getInstance();
    db.shutdown();
    remove(TEST_DB_PATH.c_str());
}

// 辅助函数：应用插入操作并返回新版本号
uint64_t applyInsertOperation(DocumentState& state, const std::string& content, uint64_t version) {
    Operation op;
    op.opId = "op_" + std::to_string(version);
    op.userId = "user_test";
    op.version = version;
    op.type = OperationType::INSERT;
    op.position = state.getContent().length();
    op.content = content;
    
    auto result = state.applyOperation(op);
    if (result != OperationResult::SUCCESS) {
        throw std::runtime_error("Failed to apply operation at version " + std::to_string(version));
    }
    
    return state.getVersion();
}

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

// 测试4: 创建和加载快照（真实数据库）
TEST(SnapshotManagerTest, CreateAndLoadSnapshot) {
    initTestDatabase();
    
    auto& manager = SnapshotManager::getInstance();
    
    // 创建文档状态并应用操作以增加版本
    DocumentState state("doc_test_001");
    state.setContent("Hello");
    
    // 应用一个插入操作使版本变为1
    Operation op;
    op.opId = "op_001";
    op.userId = "user_001";
    op.version = 1;
    op.type = OperationType::INSERT;
    op.position = 5;
    op.content = " World";
    
    auto result = state.applyOperation(op);
    ASSERT_EQ(result, OperationResult::SUCCESS);
    ASSERT_EQ(state.getVersion(), 1);
    
    // 创建快照
    bool snapshotResult = manager.createSnapshot("doc_test_001", state);
    ASSERT_TRUE(snapshotResult);
    
    // 加载快照数据
    SnapshotData data = manager.loadSnapshotData("doc_test_001");
    
    ASSERT_STREQ("doc_test_001", data.docId.c_str());
    ASSERT_GE(data.version, 1);
    ASSERT_STREQ("Hello World", data.content.c_str());
    
    cleanupTestDatabase();
}

// 测试5: 多个快照版本管理
TEST(SnapshotManagerTest, MultipleSnapshotVersions) {
    initTestDatabase();
    
    auto& manager = SnapshotManager::getInstance();
    
    // 创建多个版本的快照
    for (int i = 1; i <= 5; ++i) {
        DocumentState state("doc_multi");
        state.setContent("Base");
        
        // 应用操作使版本号递增
        for (uint64_t v = 1; v <= static_cast<uint64_t>(i); ++v) {
            applyInsertOperation(state, "X", v);
        }
        
        bool result = manager.createSnapshot("doc_multi", state);
        ASSERT_TRUE(result);
    }
    
    // 获取最新版本
    uint64_t latestVersion = manager.getLatestSnapshotVersion("doc_multi");
    ASSERT_GE(latestVersion, 5);
    
    // 加载最新快照
    SnapshotData data = manager.loadSnapshotData("doc_multi");
    ASSERT_GE(data.version, 5);
    ASSERT_STREQ("BaseXXXXX", data.content.c_str());
    
    cleanupTestDatabase();
}

// 测试6: 快照清理功能
TEST(SnapshotManagerTest, CleanupOldSnapshots) {
    initTestDatabase();
    
    auto& manager = SnapshotManager::getInstance();
    
    // 创建10个快照，每个有不同的版本号
    for (int i = 1; i <= 10; ++i) {
        DocumentState state("doc_cleanup");
        state.setContent("Base");
        
        // 应用操作使版本号为i*100
        for (uint64_t v = 1; v <= static_cast<uint64_t>(i * 100); ++v) {
            applyInsertOperation(state, ".", v);
        }
        
        manager.createSnapshot("doc_cleanup", state);
    }
    
    // 清理，保留最近5个
    int deletedCount = manager.cleanupOldSnapshots("doc_cleanup", 5);
    ASSERT_GE(deletedCount, 5);
    
    // 验证剩余快照数量
    uint64_t latestVersion = manager.getLatestSnapshotVersion("doc_cleanup");
    ASSERT_GE(latestVersion, 600); // 至少保留到版本600
    
    cleanupTestDatabase();
}

// 测试7: 空文档快照
TEST(SnapshotManagerTest, EmptyDocumentSnapshot) {
    initTestDatabase();
    
    auto& manager = SnapshotManager::getInstance();
    
    DocumentState state("doc_empty");
    state.setContent("");
    
    // 应用一个操作使版本为1
    applyInsertOperation(state, "", 1);
    
    bool result = manager.createSnapshot("doc_empty", state);
    ASSERT_TRUE(result);
    
    SnapshotData data = manager.loadSnapshotData("doc_empty");
    ASSERT_STREQ("doc_empty", data.docId.c_str());
    ASSERT_GE(data.version, 1);
    ASSERT_TRUE(data.content.empty());
    
    cleanupTestDatabase();
}

// 测试8: 不存在文档的快照加载
TEST(SnapshotManagerTest, LoadNonExistentSnapshot) {
    initTestDatabase();
    
    auto& manager = SnapshotManager::getInstance();
    
    SnapshotData data = manager.loadSnapshotData("doc_nonexistent");
    
    ASSERT_STREQ("doc_nonexistent", data.docId.c_str());
    ASSERT_EQ(data.version, 0);
    ASSERT_TRUE(data.content.empty());
    
    cleanupTestDatabase();
}

// 测试9: 多文档快照隔离
TEST(SnapshotManagerTest, MultiDocumentIsolation) {
    initTestDatabase();
    
    auto& manager = SnapshotManager::getInstance();
    
    // 为不同文档创建快照
    DocumentState state1("doc_A");
    state1.setContent("Content A");
    applyInsertOperation(state1, "", 1);
    manager.createSnapshot("doc_A", state1);
    
    DocumentState state2("doc_B");
    state2.setContent("Content B");
    applyInsertOperation(state2, "", 1);
    manager.createSnapshot("doc_B", state2);
    
    // 验证隔离
    SnapshotData dataA = manager.loadSnapshotData("doc_A");
    SnapshotData dataB = manager.loadSnapshotData("doc_B");
    
    ASSERT_STREQ("Content A", dataA.content.c_str());
    ASSERT_STREQ("Content B", dataB.content.c_str());
    
    cleanupTestDatabase();
}

// 测试10: 大文档快照性能
TEST(SnapshotManagerTest, LargeDocumentSnapshot) {
    initTestDatabase();
    
    auto& manager = SnapshotManager::getInstance();
    
    // 创建大文档内容（100KB）
    std::string largeContent(100 * 1024, 'A');
    
    DocumentState state("doc_large");
    state.setContent(largeContent);
    applyInsertOperation(state, "", 1);
    
    bool result = manager.createSnapshot("doc_large", state);
    ASSERT_TRUE(result);
    
    SnapshotData data = manager.loadSnapshotData("doc_large");
    ASSERT_EQ(data.content.size(), largeContent.size());
    
    cleanupTestDatabase();
}

// 测试18: 并发快照创建（模拟）
TEST(SnapshotManagerTest, ConcurrentSnapshotCreation) {
    initTestDatabase();
    
    auto& manager = SnapshotManager::getInstance();
    
    // 模拟并发创建（实际项目中应使用多线程测试）
    for (int i = 0; i < 10; ++i) {
        DocumentState state("doc_concurrent_" + std::to_string(i));
        state.setContent("Content " + std::to_string(i));
        applyInsertOperation(state, "", 1);
        
        bool result = manager.createSnapshot("doc_concurrent_" + std::to_string(i), state);
        ASSERT_TRUE(result);
    }
    
    cleanupTestDatabase();
}

// 测试19: 快照版本递增验证
TEST(SnapshotManagerTest, SnapshotVersionIncrement) {
    initTestDatabase();
    
    auto& manager = SnapshotManager::getInstance();
    
    // 创建递增版本的快照
    for (int i = 1; i <= 5; ++i) {
        DocumentState state("doc_increment");
        state.setContent("v" + std::to_string(i));
        
        // 应用操作使版本号为i
        for (uint64_t v = 1; v <= static_cast<uint64_t>(i); ++v) {
            applyInsertOperation(state, ".", v);
        }
        
        manager.createSnapshot("doc_increment", state);
    }
    
    // 最新版本应该>=5
    uint64_t latest = manager.getLatestSnapshotVersion("doc_increment");
    ASSERT_GE(latest, 5);
    
    cleanupTestDatabase();
}

// ==================== RecoveryManager Tests ====================

// 测试11: RecoveryManager单例
TEST(RecoveryManagerTest, SingletonInstance) {
    auto& manager1 = RecoveryManager::getInstance();
    auto& manager2 = RecoveryManager::getInstance();
    
    ASSERT_EQ(&manager1, &manager2);
}

// 测试12: 恢复不存在的文档
TEST(RecoveryManagerTest, RecoverNonExistentDocument) {
    initTestDatabase();
    
    auto& recoveryManager = RecoveryManager::getInstance();
    
    auto state = recoveryManager.recoverDocument("doc_nonexistent");
    
    ASSERT_NE(state, nullptr);
    ASSERT_STREQ("doc_nonexistent", state->getDocId().c_str());
    ASSERT_TRUE(state->getContent().empty());
    
    cleanupTestDatabase();
}

// 测试13: 从快照恢复文档
TEST(RecoveryManagerTest, RecoverFromSnapshot) {
    initTestDatabase();
    
    // 先创建快照
    auto& snapshotManager = SnapshotManager::getInstance();
    DocumentState snapState("doc_recovery");
    snapState.setContent("Base Content");
    applyInsertOperation(snapState, "", 1);
    snapshotManager.createSnapshot("doc_recovery", snapState);
    
    // 恢复文档
    auto& recoveryManager = RecoveryManager::getInstance();
    auto recoveredState = recoveryManager.recoverDocument("doc_recovery");
    
    ASSERT_NE(recoveredState, nullptr);
    ASSERT_STREQ("doc_recovery", recoveredState->getDocId().c_str());
    ASSERT_STREQ("Base Content", recoveredState->getContent().c_str());
    // 注意：版本号从0开始，因为没有应用操作
    
    cleanupTestDatabase();
}

// 测试14: 恢复到指定版本
TEST(RecoveryManagerTest, RecoverToSpecificVersion) {
    initTestDatabase();
    
    auto& snapshotManager = SnapshotManager::getInstance();
    auto& recoveryManager = RecoveryManager::getInstance();
    
    // 创建基础快照
    DocumentState baseState("doc_versioned");
    baseState.setContent("v1");
    applyInsertOperation(baseState, "", 1);
    snapshotManager.createSnapshot("doc_versioned", baseState);
    
    // 恢复到版本100（没有后续操作，应该返回快照状态）
    auto recoveredState = recoveryManager.recoverToVersion("doc_versioned", 100);
    
    ASSERT_NE(recoveredState, nullptr);
    ASSERT_STREQ("doc_versioned", recoveredState->getDocId().c_str());
    ASSERT_STREQ("v1", recoveredState->getContent().c_str());
    
    cleanupTestDatabase();
}

// 测试15: 快照+操作联合恢复
TEST(RecoveryManagerTest, SnapshotWithOperations) {
    initTestDatabase();
    
    auto& snapshotManager = SnapshotManager::getInstance();
    auto& recoveryManager = RecoveryManager::getInstance();
    auto& db = Database::getInstance();
    
    // 创建快照（版本100）
    DocumentState snapshotState("doc_combined");
    snapshotState.setContent("Snapshot at v100");
    
    // 应用100个操作使版本达到100
    for (uint64_t v = 1; v <= 100; ++v) {
        applyInsertOperation(snapshotState, ".", v);
    }
    
    snapshotManager.createSnapshot("doc_combined", snapshotState);
    
    // 插入一些后续操作到数据库（版本101-105）
    std::string insertOp = R"(
        INSERT INTO operations (doc_id, op_id, user_id, version, op_type, position, content, timestamp)
        VALUES (?, ?, ?, ?, ?, ?, ?, datetime('now'))
    )";
    
    for (int i = 101; i <= 105; ++i) {
        std::vector<std::string> params = {
            "doc_combined",
            "op_" + std::to_string(i),
            "user_test",
            std::to_string(i),
            "INSERT",
            std::to_string(snapshotState.getContent().length()),
            "Text" + std::to_string(i)
        };
        db.executeQuery(insertOp, params);
    }
    
    // 恢复文档（应该包含快照+5个操作）
    auto recoveredState = recoveryManager.recoverDocument("doc_combined");
    
    ASSERT_NE(recoveredState, nullptr);
    // 检查内容是否包含快照内容和后续操作的内容
    ASSERT_TRUE(recoveredState->getContent().find("Snapshot at v100") != std::string::npos);
    // 应该应用了5个操作，每个添加"TextXXX"
    ASSERT_GE(recoveredState->getContent().length(), snapshotState.getContent().length());
    
    cleanupTestDatabase();
}

// 测试16: 大量快照性能测试
TEST(RecoveryManagerTest, PerformanceWithManySnapshots) {
    initTestDatabase();
    
    auto& snapshotManager = SnapshotManager::getInstance();
    auto& recoveryManager = RecoveryManager::getInstance();
    
    // 创建50个快照，每个有不同的版本
    for (int i = 1; i <= 50; ++i) {
        DocumentState state("doc_performance");
        state.setContent("Content v" + std::to_string(i));
        
        // 应用操作使版本号为i
        for (uint64_t v = 1; v <= static_cast<uint64_t>(i); ++v) {
            applyInsertOperation(state, ".", v);
        }
        
        snapshotManager.createSnapshot("doc_performance", state);
    }
    
    // 获取最新版本
    uint64_t latestVersion = snapshotManager.getLatestSnapshotVersion("doc_performance");
    ASSERT_GE(latestVersion, 50);
    
    // 恢复应该很快
    auto recoveredState = recoveryManager.recoverDocument("doc_performance");
    ASSERT_NE(recoveredState, nullptr);
    // 检查内容是否正确恢复
    ASSERT_TRUE(recoveredState->getContent().find("Content v50") != std::string::npos);
    
    cleanupTestDatabase();
}

// 测试17: 快照清理后恢复
TEST(RecoveryManagerTest, RecoveryAfterCleanup) {
    initTestDatabase();
    
    auto& snapshotManager = SnapshotManager::getInstance();
    auto& recoveryManager = RecoveryManager::getInstance();
    
    // 创建10个快照，版本分别为100,200,...,1000
    for (int i = 1; i <= 10; ++i) {
        DocumentState state("doc_cleanup_recovery");
        state.setContent("v" + std::to_string(i));
        
        // 应用操作使版本号为i*100
        for (uint64_t v = 1; v <= static_cast<uint64_t>(i * 100); ++v) {
            applyInsertOperation(state, ".", v);
        }
        
        snapshotManager.createSnapshot("doc_cleanup_recovery", state);
    }
    
    // 清理，保留最近3个
    snapshotManager.cleanupOldSnapshots("doc_cleanup_recovery", 3);
    
    // 恢复应该使用最新的快照（版本1000，内容为"v10" + 1000个点）
    auto recoveredState = recoveryManager.recoverDocument("doc_cleanup_recovery");
    
    ASSERT_NE(recoveredState, nullptr);
    // 检查是否恢复了最新的内容（以"v10"开头）
    ASSERT_TRUE(recoveredState->getContent().find("v10") == 0);
    // 内容长度应该是 "v10"(3字符) + 1000个点 = 1003
    ASSERT_GE(recoveredState->getContent().length(), 1000);
    
    cleanupTestDatabase();
}

// 测试20: RecoveryManager错误处理
TEST(RecoveryManagerTest, RecoveryErrorHandling) {
    initTestDatabase();
    
    auto& recoveryManager = RecoveryManager::getInstance();
    
    // 尝试恢复无效文档ID（应该不会崩溃）
    auto state = recoveryManager.recoverDocument("");
    
    ASSERT_NE(state, nullptr);
    // 应该返回空状态
    ASSERT_TRUE(state->getDocId().empty() || state->getVersion() == 0);
    
    cleanupTestDatabase();
}

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Running Persistence & Recovery Tests" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    test::TestRunner::getInstance().runAll();
    return 0;
}

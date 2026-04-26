#include "test_framework.h"
#include "storage/Database.h"
#include "storage/DocumentDAO.h"
#include "storage/OperationDAO.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace storage;

// 测试主函数
int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Running Storage Tests" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // 运行所有已注册的测试
    test::TestRunner::getInstance().runAll();
    
    return 0;
}

// 测试1: 数据库初始化和关闭
TEST(DatabaseTest, InitializationAndShutdown) {
    DatabaseConfig config;
    config.dbPath = "test_init.db";
    config.poolSize = 2;
    
    auto& db = Database::getInstance();
    ASSERT_TRUE(db.initialize(config));
    ASSERT_TRUE(db.isInitialized());
    ASSERT_EQ(2, db.getPoolSize());
    
    db.shutdown();
    ASSERT_FALSE(db.isInitialized());
    
    // 清理测试文件
    std::remove("test_init.db");
    std::remove("test_init.db-wal");
    std::remove("test_init.db-shm");
}

// 测试2: 连接池管理
TEST(DatabaseTest, ConnectionPool) {
    DatabaseConfig config;
    config.dbPath = "test_pool.db";
    config.poolSize = 3;
    
    auto& db = Database::getInstance();
    ASSERT_TRUE(db.initialize(config));
    
    // 获取多个连接
    sqlite3* conn1 = db.getConnection();
    sqlite3* conn2 = db.getConnection();
    sqlite3* conn3 = db.getConnection();
    
    ASSERT_NE(nullptr, conn1);
    ASSERT_NE(nullptr, conn2);
    ASSERT_NE(nullptr, conn3);
    
    // 释放连接
    db.releaseConnection(conn1);
    db.releaseConnection(conn2);
    db.releaseConnection(conn3);
    
    db.shutdown();
    
    // 清理测试文件
    std::remove("test_pool.db");
    std::remove("test_pool.db-wal");
    std::remove("test_pool.db-shm");
}

// 测试3: SQL查询执行
TEST(DatabaseTest, QueryExecution) {
    DatabaseConfig config;
    config.dbPath = "test_query.db";
    config.poolSize = 1;
    
    auto& db = Database::getInstance();
    ASSERT_TRUE(db.initialize(config));
    
    // 创建测试表
    ASSERT_TRUE(db.executeQuery("CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT)"));
    
    // 插入数据
    ASSERT_TRUE(db.executeQuery("INSERT INTO test (name) VALUES ('Alice')"));
    ASSERT_TRUE(db.executeQuery("INSERT INTO test (name) VALUES ('Bob')"));
    
    // 查询单行
    std::vector<std::string> result;
    ASSERT_TRUE(db.querySingleRow("SELECT name FROM test WHERE id = ?", {"1"}, result));
    ASSERT_EQ(1, result.size());
    ASSERT_STREQ("Alice", result[0].c_str());
    
    // 查询多行
    std::vector<std::vector<std::string>> results;
    ASSERT_TRUE(db.queryMultipleRows("SELECT name FROM test ORDER BY id", {}, results));
    ASSERT_EQ(2, results.size());
    ASSERT_STREQ("Alice", results[0][0].c_str());
    ASSERT_STREQ("Bob", results[1][0].c_str());
    
    db.shutdown();
    
    // 清理测试文件
    std::remove("test_query.db");
    std::remove("test_query.db-wal");
    std::remove("test_query.db-shm");
}

// 测试4: 事务处理
TEST(DatabaseTest, TransactionHandling) {
    DatabaseConfig config;
    config.dbPath = "test_transaction.db";
    config.poolSize = 1;
    
    auto& db = Database::getInstance();
    ASSERT_TRUE(db.initialize(config));
    
    // 创建测试表
    ASSERT_TRUE(db.executeQuery("CREATE TABLE accounts (id INTEGER PRIMARY KEY, balance INTEGER)"));
    
    // 开始事务
    ASSERT_TRUE(db.beginTransaction());
    
    // 插入数据
    ASSERT_TRUE(db.executeQuery("INSERT INTO accounts (balance) VALUES (100)"));
    ASSERT_TRUE(db.executeQuery("INSERT INTO accounts (balance) VALUES (200)"));
    
    // 提交事务
    ASSERT_TRUE(db.commitTransaction());
    
    // 验证数据
    std::vector<std::vector<std::string>> results;
    ASSERT_TRUE(db.queryMultipleRows("SELECT balance FROM accounts ORDER BY id", {}, results));
    ASSERT_EQ(2, results.size());
    
    db.shutdown();
    
    // 清理测试文件
    std::remove("test_transaction.db");
    std::remove("test_transaction.db-wal");
    std::remove("test_transaction.db-shm");
}

// 测试5: DocumentDAO - 创建和获取文档
TEST(DocumentDAOTest, CreateAndGetDocument) {
    DatabaseConfig config;
    config.dbPath = "test_doc_crud.db";
    config.poolSize = 1;
    
    auto& db = Database::getInstance();
    ASSERT_TRUE(db.initialize(config));
    
    // 先创建测试用户
    ASSERT_TRUE(db.executeQuery("INSERT OR IGNORE INTO users (user_id, username) VALUES ('user_001', 'test_user')"));
    
    DocumentDAO docDAO(db);
    
    // 创建文档
    Document doc;
    doc.docId = "doc_test_001";
    doc.title = "Test Document";
    doc.ownerId = "user_001";
    doc.content = "Hello, World!";
    doc.currentVersion = 0;
    
    ASSERT_TRUE(docDAO.createDocument(doc));
    
    // 获取文档
    Document retrieved = docDAO.getDocument("doc_test_001");
    ASSERT_STREQ("doc_test_001", retrieved.docId.c_str());
    ASSERT_STREQ("Test Document", retrieved.title.c_str());
    ASSERT_STREQ("user_001", retrieved.ownerId.c_str());
    ASSERT_STREQ("Hello, World!", retrieved.content.c_str());
    ASSERT_EQ(0, retrieved.currentVersion);
    
    db.shutdown();
    
    // 清理测试文件
    std::remove("test_doc_crud.db");
    std::remove("test_doc_crud.db-wal");
    std::remove("test_doc_crud.db-shm");
}

// 测试6: DocumentDAO - 更新文档
TEST(DocumentDAOTest, UpdateDocument) {
    DatabaseConfig config;
    config.dbPath = "test_doc_update.db";
    config.poolSize = 1;
    
    auto& db = Database::getInstance();
    ASSERT_TRUE(db.initialize(config));
    
    DocumentDAO docDAO(db);
    
    // 创建文档
    Document doc;
    doc.docId = "doc_test_002";
    doc.title = "Original Title";
    doc.ownerId = "user_002";
    doc.content = "Original content";
    doc.currentVersion = 0;
    
    ASSERT_TRUE(docDAO.createDocument(doc));
    
    // 更新文档
    doc.title = "Updated Title";
    doc.content = "Updated content";
    doc.currentVersion = 1;
    
    ASSERT_TRUE(docDAO.updateDocument(doc));
    
    // 验证更新
    Document retrieved = docDAO.getDocument("doc_test_002");
    ASSERT_STREQ("Updated Title", retrieved.title.c_str());
    ASSERT_STREQ("Updated content", retrieved.content.c_str());
    ASSERT_EQ(1, retrieved.currentVersion);
    
    db.shutdown();
    
    // 清理测试文件
    std::remove("test_doc_update.db");
    std::remove("test_doc_update.db-wal");
    std::remove("test_doc_update.db-shm");
}

// 测试7: DocumentDAO - 删除文档
TEST(DocumentDAOTest, DeleteDocument) {
    DatabaseConfig config;
    config.dbPath = "test_doc_delete.db";
    config.poolSize = 1;
    
    auto& db = Database::getInstance();
    ASSERT_TRUE(db.initialize(config));
    
    DocumentDAO docDAO(db);
    
    // 创建文档
    Document doc;
    doc.docId = "doc_test_003";
    doc.title = "To Be Deleted";
    doc.ownerId = "user_003";
    
    ASSERT_TRUE(docDAO.createDocument(doc));
    
    // 软删除
    ASSERT_TRUE(docDAO.deleteDocument("doc_test_003"));
    
    // 检查是否存在（应该仍然存在，但标记为删除）
    ASSERT_TRUE(docDAO.exists("doc_test_003"));
    
    // 永久删除
    ASSERT_TRUE(docDAO.permanentlyDeleteDocument("doc_test_003"));
    
    // 再次检查（应该不存在了）
    ASSERT_FALSE(docDAO.exists("doc_test_003"));
    
    db.shutdown();
    
    // 清理测试文件
    std::remove("test_doc_delete.db");
    std::remove("test_doc_delete.db-wal");
    std::remove("test_doc_delete.db-shm");
}

// 测试8: DocumentDAO - 获取用户文档列表
TEST(DocumentDAOTest, GetUserDocuments) {
    DatabaseConfig config;
    config.dbPath = "test_doc_list.db";
    config.poolSize = 1;
    
    auto& db = Database::getInstance();
    ASSERT_TRUE(db.initialize(config));
    
    DocumentDAO docDAO(db);
    
    // 创建多个文档
    for (int i = 1; i <= 3; ++i) {
        Document doc;
        doc.docId = "doc_user_" + std::to_string(i);
        doc.title = "Document " + std::to_string(i);
        doc.ownerId = "user_004";
        doc.content = "Content " + std::to_string(i);
        
        ASSERT_TRUE(docDAO.createDocument(doc));
    }
    
    // 获取用户文档列表
    auto docs = docDAO.getUserDocuments("user_004");
    ASSERT_EQ(3, docs.size());
    
    db.shutdown();
    
    // 清理测试文件
    std::remove("test_doc_list.db");
    std::remove("test_doc_list.db-wal");
    std::remove("test_doc_list.db-shm");
}

// 测试9: OperationDAO - 插入和获取操作
TEST(OperationDAOTest, InsertAndGetOperation) {
    DatabaseConfig config;
    config.dbPath = "test_op_crud.db";
    config.poolSize = 1;
    
    auto& db = Database::getInstance();
    ASSERT_TRUE(db.initialize(config));
    
    OperationDAO opDAO(db);
    
    // 创建操作
    Operation op;
    op.docId = "doc_001";
    op.opId = "op_test_001";
    op.userId = "user_001";
    op.version = 1;
    op.opType = "insert";
    op.position = 0;
    op.length = 5;
    op.content = "Hello";
    
    ASSERT_TRUE(opDAO.insertOperation(op));
    
    // 获取操作
    Operation retrieved = opDAO.getOperation("op_test_001");
    ASSERT_STREQ("op_test_001", retrieved.opId.c_str());
    ASSERT_STREQ("doc_001", retrieved.docId.c_str());
    ASSERT_STREQ("insert", retrieved.opType.c_str());
    ASSERT_EQ(1, retrieved.version);
    ASSERT_STREQ("Hello", retrieved.content.c_str());
    
    db.shutdown();
    
    // 清理测试文件
    std::remove("test_op_crud.db");
    std::remove("test_op_crud.db-wal");
    std::remove("test_op_crud.db-shm");
}

// 测试10: OperationDAO - 获取文档操作列表
TEST(OperationDAOTest, GetDocumentOperations) {
    DatabaseConfig config;
    config.dbPath = "test_op_list.db";
    config.poolSize = 1;
    
    auto& db = Database::getInstance();
    ASSERT_TRUE(db.initialize(config));
    
    OperationDAO opDAO(db);
    
    // 插入多个操作
    for (int i = 1; i <= 5; ++i) {
        Operation op;
        op.docId = "doc_002";
        op.opId = "op_" + std::to_string(i);
        op.userId = "user_002";
        op.version = i;
        op.opType = "insert";
        op.position = 0;
        op.length = 1;
        op.content = "X";
        
        ASSERT_TRUE(opDAO.insertOperation(op));
    }
    
    // 获取所有操作
    auto ops = opDAO.getDocumentOperations("doc_002");
    ASSERT_EQ(5, ops.size());
    
    // 验证顺序
    ASSERT_EQ(1, ops[0].version);
    ASSERT_EQ(5, ops[4].version);
    
    db.shutdown();
    
    // 清理测试文件
    std::remove("test_op_list.db");
    std::remove("test_op_list.db-wal");
    std::remove("test_op_list.db-shm");
}

// 测试11: OperationDAO - 版本范围查询
TEST(OperationDAOTest, GetOperationsByVersionRange) {
    DatabaseConfig config;
    config.dbPath = "test_op_version.db";
    config.poolSize = 1;
    
    auto& db = Database::getInstance();
    ASSERT_TRUE(db.initialize(config));
    
    OperationDAO opDAO(db);
    
    // 插入10个操作
    for (int i = 1; i <= 10; ++i) {
        Operation op;
        op.docId = "doc_003";
        op.opId = "op_v" + std::to_string(i);
        op.userId = "user_003";
        op.version = i;
        op.opType = "insert";
        op.position = 0;
        op.length = 1;
        op.content = "X";
        
        ASSERT_TRUE(opDAO.insertOperation(op));
    }
    
    // 获取版本3-7的操作
    auto ops = opDAO.getOperationsByVersion("doc_003", 3, 7);
    ASSERT_EQ(5, ops.size());
    ASSERT_EQ(3, ops[0].version);
    ASSERT_EQ(7, ops[4].version);
    
    db.shutdown();
    
    // 清理测试文件
    std::remove("test_op_version.db");
    std::remove("test_op_version.db-wal");
    std::remove("test_op_version.db-shm");
}

// 测试12: OperationDAO - 当前版本查询
TEST(OperationDAOTest, GetCurrentVersion) {
    DatabaseConfig config;
    config.dbPath = "test_op_current.db";
    config.poolSize = 1;
    
    auto& db = Database::getInstance();
    ASSERT_TRUE(db.initialize(config));
    
    OperationDAO opDAO(db);
    
    // 初始版本应该是0
    ASSERT_EQ(0, opDAO.getCurrentVersion("doc_004"));
    
    // 插入操作
    Operation op;
    op.docId = "doc_004";
    op.opId = "op_001";
    op.userId = "user_004";
    op.version = 5;
    op.opType = "insert";
    
    ASSERT_TRUE(opDAO.insertOperation(op));
    
    // 当前版本应该是5
    ASSERT_EQ(5, opDAO.getCurrentVersion("doc_004"));
    
    db.shutdown();
    
    // 清理测试文件
    std::remove("test_op_current.db");
    std::remove("test_op_current.db-wal");
    std::remove("test_op_current.db-shm");
}

// 测试13: 权限检查
TEST(DocumentDAOTest, PermissionCheck) {
    DatabaseConfig config;
    config.dbPath = "test_permission.db";
    config.poolSize = 1;
    
    auto& db = Database::getInstance();
    ASSERT_TRUE(db.initialize(config));
    
    DocumentDAO docDAO(db);
    
    // 创建文档
    Document doc;
    doc.docId = "doc_perm_001";
    doc.title = "Permission Test";
    doc.ownerId = "owner_001";
    
    ASSERT_TRUE(docDAO.createDocument(doc));
    
    // 检查所有者权限
    ASSERT_TRUE(docDAO.isOwner("doc_perm_001", "owner_001"));
    ASSERT_FALSE(docDAO.isOwner("doc_perm_001", "other_user"));
    
    // 检查访问权限
    ASSERT_TRUE(docDAO.hasAccess("doc_perm_001", "owner_001"));
    ASSERT_FALSE(docDAO.hasAccess("doc_perm_001", "other_user"));
    
    db.shutdown();
    
    // 清理测试文件
    std::remove("test_permission.db");
    std::remove("test_permission.db-wal");
    std::remove("test_permission.db-shm");
}

// 测试14: 并发连接测试
TEST(DatabaseTest, ConcurrentConnections) {
    DatabaseConfig config;
    config.dbPath = "test_concurrent.db";
    config.poolSize = 2;
    
    auto& db = Database::getInstance();
    ASSERT_TRUE(db.initialize(config));
    
    // 创建测试表（使用IF NOT EXISTS）
    ASSERT_TRUE(db.executeQuery("CREATE TABLE IF NOT EXISTS counter (value INTEGER)"));
    ASSERT_TRUE(db.executeQuery("INSERT OR IGNORE INTO counter (value) VALUES (0)"));
    
    // 模拟并发访问
    auto threadFunc = [&db]() {
        for (int i = 0; i < 10; ++i) {
            sqlite3* conn = db.getConnection();
            if (conn) {
                // 模拟一些工作
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                db.releaseConnection(conn);
            }
        }
    };
    
    std::thread t1(threadFunc);
    std::thread t2(threadFunc);
    
    t1.join();
    t2.join();
    
    db.shutdown();
    
    // 清理测试文件
    std::remove("test_concurrent.db");
    std::remove("test_concurrent.db-wal");
    std::remove("test_concurrent.db-shm");
}

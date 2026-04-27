#include "test_framework.h"
#include "core/Operation.h"
#include "core/DocumentState.h"
#include "core/OTAlgorithm.h"

using namespace core;

// ==================== Operation Tests ====================

// 测试1: Operation默认构造
TEST(OperationTest, DefaultConstructor) {
    Operation op;
    ASSERT_EQ(op.version, 0);
    ASSERT_EQ(op.type, OperationType::RETAIN);
    ASSERT_EQ(op.position, 0);
    ASSERT_TRUE(op.content.empty());
}

// 测试2: Operation带参构造
TEST(OperationTest, ParameterizedConstructor) {
    Operation op("op_001", "user_001", 1, OperationType::INSERT, 5, "Hello");
    ASSERT_STREQ("op_001", op.opId.c_str());
    ASSERT_STREQ("user_001", op.userId.c_str());
    ASSERT_EQ(op.version, 1);
    ASSERT_EQ(op.type, OperationType::INSERT);
    ASSERT_EQ(op.position, 5);
    ASSERT_STREQ("Hello", op.content.c_str());
}

// 测试3: Operation序列化
TEST(OperationTest, SerializeToJson) {
    Operation op("op_001", "user_001", 1, OperationType::INSERT, 5, "Hello");
    std::string json = op.toJson();
    
    ASSERT_FALSE(json.empty());
    ASSERT_TRUE(json.find("op_001") != std::string::npos);
    ASSERT_TRUE(json.find("INSERT") != std::string::npos || json.find("insert") != std::string::npos);
}

// 测试4: Operation反序列化
TEST(OperationTest, DeserializeFromJson) {
    std::string json = R"({
        "opId": "op_002",
        "userId": "user_002",
        "version": 2,
        "type": "delete",
        "position": 10,
        "content": "World"
    })";
    
    Operation op = Operation::fromJson(json);
    ASSERT_STREQ("op_002", op.opId.c_str());
    ASSERT_STREQ("user_002", op.userId.c_str());
    ASSERT_EQ(op.version, 2);
    ASSERT_EQ(op.type, OperationType::DELETE);
    ASSERT_EQ(op.position, 10);
    ASSERT_STREQ("World", op.content.c_str());
}

// 测试5: Operation类型转换
TEST(OperationTest, TypeConversion) {
    ASSERT_STREQ("insert", operationTypeToString(OperationType::INSERT).c_str());
    ASSERT_STREQ("delete", operationTypeToString(OperationType::DELETE).c_str());
    ASSERT_STREQ("replace", operationTypeToString(OperationType::REPLACE).c_str());
    ASSERT_STREQ("retain", operationTypeToString(OperationType::RETAIN).c_str());
    
    ASSERT_EQ(OperationType::INSERT, stringToOperationType("insert"));
    ASSERT_EQ(OperationType::DELETE, stringToOperationType("delete"));
    ASSERT_EQ(OperationType::REPLACE, stringToOperationType("replace"));
    ASSERT_EQ(OperationType::RETAIN, stringToOperationType("retain"));
}

// ==================== DocumentState Tests ====================

// 测试6: DocumentState创建
TEST(DocumentStateTest, Creation) {
    DocumentState doc("doc_001");
    ASSERT_STREQ("doc_001", doc.getDocId().c_str());
    ASSERT_EQ(doc.getVersion(), 0);
    ASSERT_TRUE(doc.getContent().empty());
}

// 测试7: 设置和获取内容
TEST(DocumentStateTest, SetAndGetContent) {
    DocumentState doc("doc_001");
    doc.setContent("Hello World");
    ASSERT_STREQ("Hello World", doc.getContent().c_str());
}

// 测试8: 应用INSERT操作
TEST(DocumentStateTest, ApplyInsertOperation) {
    DocumentState doc("doc_001");
    doc.setContent("Hello World");
    
    Operation op("op_001", "user_001", 1, OperationType::INSERT, 5, " Beautiful");
    OperationResult result = doc.applyOperation(op);
    
    ASSERT_EQ(result, OperationResult::SUCCESS);
    ASSERT_STREQ("Hello Beautiful World", doc.getContent().c_str());
    ASSERT_EQ(doc.getVersion(), 1);
}

// 测试9: 应用DELETE操作
TEST(DocumentStateTest, ApplyDeleteOperation) {
    DocumentState doc("doc_001");
    doc.setContent("Hello Beautiful World");
    
    Operation op("op_001", "user_001", 1, OperationType::DELETE, 5, " Beautiful");
    OperationResult result = doc.applyOperation(op);
    
    ASSERT_EQ(result, OperationResult::SUCCESS);
    ASSERT_STREQ("Hello World", doc.getContent().c_str());
    ASSERT_EQ(doc.getVersion(), 1);
}

// 测试10: 版本号冲突检测
TEST(DocumentStateTest, VersionConflictDetection) {
    DocumentState doc("doc_001");
    doc.setContent("Hello");
    
    // 第一次应用，版本应该是1
    Operation op1("op_001", "user_001", 1, OperationType::INSERT, 5, " World");
    ASSERT_EQ(doc.applyOperation(op1), OperationResult::SUCCESS);
    
    // 再次用版本1会冲突
    Operation op2("op_002", "user_002", 1, OperationType::INSERT, 0, "Hi ");
    ASSERT_EQ(doc.applyOperation(op2), OperationResult::VERSION_CONFLICT);
}

// 测试11: 位置越界检测
TEST(DocumentStateTest, PositionOutOfRange) {
    DocumentState doc("doc_001");
    doc.setContent("Hello");
    
    // 在位置100插入（超出范围）
    Operation op("op_001", "user_001", 1, OperationType::INSERT, 100, "X");
    OperationResult result = doc.applyOperation(op);
    
    ASSERT_EQ(result, OperationResult::POSITION_OUT_OF_RANGE);
}

// 测试12: 获取操作历史
TEST(DocumentStateTest, GetHistory) {
    DocumentState doc("doc_001");
    doc.setContent("Hello");
    
    // 应用几个操作
    for (int i = 1; i <= 5; ++i) {
        Operation op("op_" + std::to_string(i), "user_001", i, 
                    OperationType::INSERT, 0, "X");
        doc.applyOperation(op);
    }
    
    auto history = doc.getHistory(3);
    ASSERT_EQ(history.size(), 3);
    ASSERT_STREQ("op_3", history[0].opId.c_str());
    ASSERT_STREQ("op_5", history[2].opId.c_str());
}

// 测试13: 空文档插入
TEST(DocumentStateTest, InsertToEmptyDocument) {
    DocumentState doc("doc_001");
    
    Operation op("op_001", "user_001", 1, OperationType::INSERT, 0, "Hello");
    OperationResult result = doc.applyOperation(op);
    
    ASSERT_EQ(result, OperationResult::SUCCESS);
    ASSERT_STREQ("Hello", doc.getContent().c_str());
}

// ==================== OT Algorithm Tests ====================

// 测试14: INSERT vs INSERT 转换（本地在后）
TEST(OTAlgorithmTest, TransformInsertInsert_LocalAfterRemote) {
    Operation local("op1", "user1", 1, OperationType::INSERT, 5, "Hello");
    Operation remote("op2", "user2", 1, OperationType::INSERT, 3, "World");
    
    Operation transformed = OTAlgorithm::transform(local, remote);
    
    // 本地位置应该后移（5 + 5 = 10）
    ASSERT_EQ(transformed.position, 10);
    ASSERT_STREQ("Hello", transformed.content.c_str());
}

// 测试15: INSERT vs INSERT 转换（本地在前）
TEST(OTAlgorithmTest, TransformInsertInsert_LocalBeforeRemote) {
    Operation local("op1", "user1", 1, OperationType::INSERT, 3, "Hello");
    Operation remote("op2", "user2", 1, OperationType::INSERT, 5, "World");
    
    Operation transformed = OTAlgorithm::transform(local, remote);
    
    // 本地位置不变
    ASSERT_EQ(transformed.position, 3);
}

// 测试16: INSERT vs DELETE 转换（本地在删除区域前）
TEST(OTAlgorithmTest, TransformInsertDelete_BeforeDelete) {
    Operation local("op1", "user1", 1, OperationType::INSERT, 2, "X");
    Operation remote("op2", "user2", 1, OperationType::DELETE, 5, "Hello");
    
    Operation transformed = OTAlgorithm::transform(local, remote);
    
    // 位置不变
    ASSERT_EQ(transformed.position, 2);
}

// 测试17: INSERT vs DELETE 转换（本地在删除区域后）
TEST(OTAlgorithmTest, TransformInsertDelete_AfterDelete) {
    Operation local("op1", "user1", 1, OperationType::INSERT, 10, "X");
    Operation remote("op2", "user2", 1, OperationType::DELETE, 5, "Hello");
    
    Operation transformed = OTAlgorithm::transform(local, remote);
    
    // 位置前移（10 - 5 = 5）
    ASSERT_EQ(transformed.position, 5);
}

// 测试18: DELETE vs INSERT 转换（本地在插入位置后）
TEST(OTAlgorithmTest, TransformDeleteInsert_AfterInsert) {
    Operation local("op1", "user1", 1, OperationType::DELETE, 10, "Hello");
    Operation remote("op2", "user2", 1, OperationType::INSERT, 5, "World");
    
    Operation transformed = OTAlgorithm::transform(local, remote);
    
    // 位置后移（10 + 5 = 15）
    ASSERT_EQ(transformed.position, 15);
}

// 测试19: DELETE vs DELETE 转换（不重叠）
TEST(OTAlgorithmTest, TransformDeleteDelete_NoOverlap) {
    Operation local("op1", "user1", 1, OperationType::DELETE, 0, "Hello");
    Operation remote("op2", "user2", 1, OperationType::DELETE, 10, "World");
    
    Operation transformed = OTAlgorithm::transform(local, remote);
    
    // 位置不变
    ASSERT_EQ(transformed.position, 0);
}

// 测试20: DELETE vs DELETE 转换（本地在远程后）
TEST(OTAlgorithmTest, TransformDeleteDelete_LocalAfterRemote) {
    Operation local("op1", "user1", 1, OperationType::DELETE, 10, "Hello");
    Operation remote("op2", "user2", 1, OperationType::DELETE, 0, "World");
    
    Operation transformed = OTAlgorithm::transform(local, remote);
    
    // 位置前移（10 - 5 = 5）
    ASSERT_EQ(transformed.position, 5);
}

// 测试21: 冲突检测 - 重叠操作
TEST(OTAlgorithmTest, ConflictDetection_Overlapping) {
    Operation op1("op1", "user1", 1, OperationType::INSERT, 5, "Hello");
    Operation op2("op2", "user2", 1, OperationType::DELETE, 3, "World");
    
    ASSERT_TRUE(OTAlgorithm::isConflict(op1, op2));
}

// 测试22: 冲突检测 - 不重叠操作
TEST(OTAlgorithmTest, ConflictDetection_NonOverlapping) {
    Operation op1("op1", "user1", 1, OperationType::INSERT, 0, "Hello");
    Operation op2("op2", "user2", 1, OperationType::DELETE, 10, "World");
    
    ASSERT_FALSE(OTAlgorithm::isConflict(op1, op2));
}

// 测试23: 冲突检测 - 同一用户
TEST(OTAlgorithmTest, ConflictDetection_SameUser) {
    Operation op1("op1", "user1", 1, OperationType::INSERT, 5, "Hello");
    Operation op2("op2", "user1", 1, OperationType::DELETE, 3, "World");
    
    // 同一用户的操作不视为冲突
    ASSERT_FALSE(OTAlgorithm::isConflict(op1, op2));
}

// 测试24: 操作往返序列化
TEST(OperationTest, RoundTripSerialization) {
    Operation original("op_test", "user_test", 42, OperationType::REPLACE, 100, "Test content");
    
    std::string json = original.toJson();
    Operation deserialized = Operation::fromJson(json);
    
    ASSERT_STREQ(original.opId.c_str(), deserialized.opId.c_str());
    ASSERT_STREQ(original.userId.c_str(), deserialized.userId.c_str());
    ASSERT_EQ(original.version, deserialized.version);
    ASSERT_EQ(original.type, deserialized.type);
    ASSERT_EQ(original.position, deserialized.position);
    ASSERT_STREQ(original.content.c_str(), deserialized.content.c_str());
}

// 测试25: 多操作应用顺序
TEST(DocumentStateTest, MultipleOperationsSequence) {
    DocumentState doc("doc_001");
    doc.setContent("Hello");
    
    // 连续应用多个操作
    Operation op1("op1", "user1", 1, OperationType::INSERT, 5, " World");
    ASSERT_EQ(doc.applyOperation(op1), OperationResult::SUCCESS);
    
    Operation op2("op2", "user2", 2, OperationType::INSERT, 11, "!");
    ASSERT_EQ(doc.applyOperation(op2), OperationResult::SUCCESS);
    
    Operation op3("op3", "user1", 3, OperationType::DELETE, 0, "Hello");
    ASSERT_EQ(doc.applyOperation(op3), OperationResult::SUCCESS);
    
    ASSERT_STREQ(" World!", doc.getContent().c_str());
    ASSERT_EQ(doc.getVersion(), 3);
}

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Running Core OT Algorithm Tests" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    test::TestRunner::getInstance().runAll();
    return 0;
}

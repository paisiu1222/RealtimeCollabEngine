#include "test_framework.h"
#include "network/Room.h"
#include "network/SessionManager.h"
#include "core/Operation.h"
#include "core/DocumentState.h"

using namespace network;
using namespace core;

// ==================== Room Tests ====================

// 测试1: Room创建
TEST(RoomTest, RoomCreation) {
    Room room("room_001", "doc_001");
    ASSERT_STREQ("room_001", room.getRoomId().c_str());
    ASSERT_STREQ("doc_001", room.getDocId().c_str());
    ASSERT_EQ(room.getUserCount(), 0);
    ASSERT_EQ(room.getVersion(), 0);
}

// 测试2: 添加用户
TEST(RoomTest, AddUser) {
    Room room("room_001", "doc_001");
    
    bool success = room.addUser("user_001", "conn_001");
    ASSERT_TRUE(success);
    ASSERT_EQ(room.getUserCount(), 1);
    ASSERT_TRUE(room.hasUser("user_001"));
}

// 测试3: 重复添加用户
TEST(RoomTest, DuplicateAddUser) {
    Room room("room_001", "doc_001");
    
    room.addUser("user_001", "conn_001");
    bool success = room.addUser("user_001", "conn_002");
    
    ASSERT_FALSE(success);
    ASSERT_EQ(room.getUserCount(), 1);
}

// 测试4: 移除用户
TEST(RoomTest, RemoveUser) {
    Room room("room_001", "doc_001");
    
    room.addUser("user_001", "conn_001");
    bool success = room.removeUser("user_001");
    
    ASSERT_TRUE(success);
    ASSERT_EQ(room.getUserCount(), 0);
    ASSERT_FALSE(room.hasUser("user_001"));
}

// 测试5: 移除不存在的用户
TEST(RoomTest, RemoveNonExistentUser) {
    Room room("room_001", "doc_001");
    
    bool success = room.removeUser("user_999");
    ASSERT_FALSE(success);
}

// 测试6: 获取用户列表
TEST(RoomTest, GetUserList) {
    Room room("room_001", "doc_001");
    
    room.addUser("user_001", "conn_001");
    room.addUser("user_002", "conn_002");
    room.addUser("user_003", "conn_003");
    
    auto users = room.getUsers();
    ASSERT_EQ(users.size(), 3);
    ASSERT_TRUE(users.count("user_001") > 0);
    ASSERT_TRUE(users.count("user_002") > 0);
    ASSERT_TRUE(users.count("user_003") > 0);
}

// 测试7: 获取用户连接ID
TEST(RoomTest, GetUserConnection) {
    Room room("room_001", "doc_001");
    
    room.addUser("user_001", "conn_001");
    std::string connId = room.getUserConnection("user_001");
    
    ASSERT_STREQ("conn_001", connId.c_str());
}

// 测试8: 应用操作并广播
TEST(RoomTest, ApplyAndBroadcast) {
    Room room("room_001", "doc_001");
    room.getDocument().setContent("Hello World");
    
    // 设置广播回调
    int broadcastCount = 0;
    room.setBroadcastCallback([&broadcastCount](const std::string& userId, const std::string& message) {
        broadcastCount++;
    });
    
    // 应用操作
    Operation op("op_001", "user_001", 1, OperationType::INSERT, 5, " Beautiful");
    auto result = room.applyAndBroadcast(op, "user_001");
    
    ASSERT_EQ(result, OperationResult::SUCCESS);
    ASSERT_EQ(room.getVersion(), 1);
    ASSERT_STREQ("Hello Beautiful World", room.getDocument().getContent().c_str());
}

// 测试9: 广播排除发送者（发送者只收到ACK，不收到操作广播）
TEST(RoomTest, BroadcastExcludesSender) {
    Room room("room_001", "doc_001");
    room.getDocument().setContent("Hello");
    
    room.addUser("user_001", "conn_001");
    room.addUser("user_002", "conn_002");
    room.addUser("user_003", "conn_003");
    
    std::map<std::string, int> messageCount;
    auto callback = [&messageCount](const std::string& userId, const std::string& /*message*/) {
        messageCount[userId]++;
    };
    room.setBroadcastCallback(callback);
    
    Operation op("op_001", "user_001", 1, OperationType::INSERT, 5, " World");
    room.applyAndBroadcast(op, "user_001");
    
    // user_001收到1条消息（ACK），user_002和user_003各收到1条广播
    ASSERT_EQ(messageCount["user_001"], 1); // ACK
    ASSERT_EQ(messageCount["user_002"], 1); // 广播
    ASSERT_EQ(messageCount["user_003"], 1); // 广播
}

// 测试10: 版本号冲突检测
TEST(RoomTest, VersionConflictDetection) {
    Room room("room_001", "doc_001");
    room.getDocument().setContent("Hello");
    
    // 第一次应用（版本1）
    Operation op1("op_001", "user_001", 1, OperationType::INSERT, 5, " World");
    ASSERT_EQ(room.applyAndBroadcast(op1, "user_001"), OperationResult::SUCCESS);
    
    // 再次用版本1会冲突
    Operation op2("op_002", "user_002", 1, OperationType::INSERT, 0, "Hi ");
    ASSERT_EQ(room.applyAndBroadcast(op2, "user_002"), OperationResult::VERSION_CONFLICT);
}

// 测试21: 多用户协作场景
TEST(RoomTest, MultiUserCollaboration) {
    Room room("room_collab", "doc_collab");
    room.getDocument().setContent("Hello");
    
    room.addUser("user_A", "conn_A");
    room.addUser("user_B", "conn_B");
    
    std::map<std::string, int> messageCount;
    auto callback = [&messageCount](const std::string& userId, const std::string& /*message*/) {
        messageCount[userId]++;
    };
    room.setBroadcastCallback(callback);
    
    // 用户A插入操作
    Operation opA("op_A", "user_A", 1, OperationType::INSERT, 5, " World");
    room.applyAndBroadcast(opA, "user_A");
    
    // user_A收到ACK，user_B收到广播
    ASSERT_EQ(messageCount["user_A"], 1); // ACK
    ASSERT_EQ(messageCount["user_B"], 1); // 广播
    
    // 用户B删除操作
    Operation opB("op_B", "user_B", 2, OperationType::DELETE, 0, "Hello");
    room.applyAndBroadcast(opB, "user_B");
    
    // user_B收到ACK，user_A收到广播
    ASSERT_EQ(messageCount["user_A"], 2); // ACK + 广播
    ASSERT_EQ(messageCount["user_B"], 2); // 广播 + ACK
}

// 测试22: 文档状态同步
TEST(RoomTest, DocumentStateSync) {
    Room room("room_sync", "doc_sync");
    room.getDocument().setContent("");
    
    // 模拟多个操作
    Operation op1("op1", "user1", 1, OperationType::INSERT, 0, "Hello");
    room.applyAndBroadcast(op1, "user1");
    
    Operation op2("op2", "user2", 2, OperationType::INSERT, 5, " World");
    room.applyAndBroadcast(op2, "user2");
    
    Operation op3("op3", "user1", 3, OperationType::INSERT, 11, "!");
    room.applyAndBroadcast(op3, "user1");
    
    ASSERT_STREQ("Hello World!", room.getDocument().getContent().c_str());
    ASSERT_EQ(room.getVersion(), 3);
}

int main() {
    test::TestRunner::getInstance().runAll();
    return 0;
}

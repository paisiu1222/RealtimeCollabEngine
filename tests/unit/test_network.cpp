#include "test_framework.h"
#include "network/MessageProtocol.h"
#include "network/WebSocketServer.h"
#include <thread>
#include <chrono>

using namespace network;

// ==================== Message Protocol Tests ====================

// 测试1: 消息序列化
TEST(MessageTest, SerializeMessage) {
    Message msg = MessageFactory::createConnectMessage("user_001", "test-token");
    
    std::string json = msg.toJson();
    ASSERT_FALSE(json.empty());
    
    // 验证JSON包含必需字段
    auto parsed = nlohmann::json::parse(json);
    ASSERT_TRUE(parsed.contains("type"));
    ASSERT_TRUE(parsed.contains("messageId"));
    ASSERT_TRUE(parsed.contains("timestamp"));
    ASSERT_TRUE(parsed.contains("payload"));
    ASSERT_STREQ("connect", parsed["type"].get<std::string>().c_str());
}

// 测试2: 消息反序列化
TEST(MessageTest, DeserializeMessage) {
    std::string json = R"({
        "type": "heartbeat",
        "messageId": "test-id-123",
        "timestamp": "2026-04-26T12:00:00.000Z",
        "payload": {"ping": true}
    })";
    
    Message msg = Message::fromJson(json);
    ASSERT_EQ(MessageType::HEARTBEAT, msg.type);
    ASSERT_STREQ("test-id-123", msg.messageId.c_str());
    ASSERT_TRUE(msg.payload.contains("ping"));
}

// 测试3: 心跳消息
TEST(MessageTest, HeartbeatMessage) {
    Message pingMsg = MessageFactory::createHeartbeatMessage();
    ASSERT_EQ(MessageType::HEARTBEAT, pingMsg.type);
    ASSERT_TRUE(pingMsg.payload.contains("ping"));
    
    Message pongMsg = MessageFactory::createHeartbeatResponse();
    ASSERT_EQ(MessageType::HEARTBEAT, pongMsg.type);
    ASSERT_TRUE(pongMsg.payload.contains("pong"));
}

// 测试4: 操作消息
TEST(MessageTest, OperationMessage) {
    Message opMsg = MessageFactory::createOperationMessage(
        "op_001", "user_001", 1, "insert", 0, "Hello"
    );
    
    ASSERT_EQ(MessageType::OPERATION, opMsg.type);
    ASSERT_STREQ("op_001", opMsg.payload["opId"].get<std::string>().c_str());
    ASSERT_EQ(1, opMsg.payload["version"]);
    ASSERT_STREQ("insert", opMsg.payload["operation"]["type"].get<std::string>().c_str());
}

// 测试5: 错误消息
TEST(MessageTest, ErrorMessage) {
    Message errorMsg = MessageFactory::createErrorMessage("AUTH_ERROR", "Invalid token");
    
    ASSERT_EQ(MessageType::ERROR, errorMsg.type);
    ASSERT_STREQ("AUTH_ERROR", errorMsg.payload["code"].get<std::string>().c_str());
    ASSERT_STREQ("Invalid token", errorMsg.payload["message"].get<std::string>().c_str());
}

// 测试6: 消息验证 - 有效消息
TEST(MessageValidatorTest, ValidMessage) {
    nlohmann::json validJson = {
        {"type", "connect"},
        {"messageId", "test-id"},
        {"timestamp", "2026-04-26T12:00:00.000Z"},
        {"payload", {{"userId", "user_001"}}}
    };
    
    ASSERT_TRUE(MessageValidator::validateBasicStructure(validJson));
}

// 测试7: 消息验证 - 缺少必需字段
TEST(MessageValidatorTest, MissingFields) {
    nlohmann::json invalidJson = {
        {"type", "connect"},
        {"messageId", "test-id"}
        // 缺少 timestamp 和 payload
    };
    
    ASSERT_FALSE(MessageValidator::validateBasicStructure(invalidJson));
}

// 测试8: 消息验证 - 无效类型
TEST(MessageValidatorTest, InvalidType) {
    nlohmann::json invalidJson = {
        {"type", "invalid_type"},
        {"messageId", "test-id"},
        {"timestamp", "2026-04-26T12:00:00.000Z"},
        {"payload", {}}
    };
    
    ASSERT_FALSE(MessageValidator::validateBasicStructure(invalidJson));
}

// 测试9: CONNECT消息payload验证
TEST(MessageValidatorTest, ConnectPayloadValidation) {
    nlohmann::json validPayload = {
        {"userId", "user_001"},
        {"token", "test-token"}
    };
    
    ASSERT_TRUE(MessageValidator::validatePayload(MessageType::CONNECT, validPayload));
    
    nlohmann::json invalidPayload = {
        {"userId", "user_001"}
        // 缺少 token
    };
    
    ASSERT_FALSE(MessageValidator::validatePayload(MessageType::CONNECT, invalidPayload));
}

// 测试10: 时间戳生成
TEST(MessageTest, TimestampGeneration) {
    std::string timestamp = Message::getCurrentTimestamp();
    ASSERT_FALSE(timestamp.empty());
    
    // 验证格式：YYYY-MM-DDTHH:MM:SS.mmmZ
    ASSERT_GT(timestamp.length(), 20);
    ASSERT_EQ('Z', timestamp.back());
}

// ==================== WebSocket Server Tests ====================

// 测试11: WebSocket服务器创建
TEST(WebSocketServerTest, ServerCreation) {
    WebSocketServer server("0.0.0.0", 9090);
    ASSERT_FALSE(server.isRunning());
}

// 测试12: 房间信息管理
TEST(WebSocketServerTest, RoomInfoManagement) {
    WebSocketServer server("0.0.0.0", 9091);
    
    // 初始时房间应该不存在
    RoomInfo info = server.getRoomInfo("nonexistent_room");
    ASSERT_STREQ("", info.roomId.c_str());
    ASSERT_EQ(0, server.getRoomUserCount("nonexistent_room"));
}

// 测试13: 在线用户计数
TEST(WebSocketServerTest, OnlineUserCount) {
    WebSocketServer server("0.0.0.0", 9092);
    
    // 初始时应该没有在线用户
    ASSERT_EQ(0, server.getOnlineUserCount());
}

// 测试14: 消息工厂 - 所有消息类型
TEST(MessageFactoryTest, AllMessageTypes) {
    // 测试各种消息类型的创建
    auto connectMsg = MessageFactory::createConnectMessage("user_001", "token");
    ASSERT_EQ(MessageType::CONNECT, connectMsg.type);
    
    auto disconnectMsg = MessageFactory::createDisconnectMessage("test");
    ASSERT_EQ(MessageType::DISCONNECT, disconnectMsg.type);
    
    auto joinRoomMsg = MessageFactory::createJoinRoomMessage("room_001");
    ASSERT_EQ(MessageType::JOIN_ROOM, joinRoomMsg.type);
    
    auto leaveRoomMsg = MessageFactory::createLeaveRoomMessage("room_001");
    ASSERT_EQ(MessageType::LEAVE_ROOM, leaveRoomMsg.type);
    
    auto roomInfoMsg = MessageFactory::createRoomInfoMessage("room_001", 5);
    ASSERT_EQ(MessageType::ROOM_INFO, roomInfoMsg.type);
    
    auto userJoinMsg = MessageFactory::createUserJoinMessage("user_001", "Alice");
    ASSERT_EQ(MessageType::USER_JOIN, userJoinMsg.type);
    
    auto userLeaveMsg = MessageFactory::createUserLeaveMessage("user_001");
    ASSERT_EQ(MessageType::USER_LEAVE, userLeaveMsg.type);
    
    auto cursorMsg = MessageFactory::createCursorUpdateMessage("user_001", 10, 0, 5);
    ASSERT_EQ(MessageType::CURSOR_UPDATE, cursorMsg.type);
    
    auto docSyncMsg = MessageFactory::createDocumentSyncMessage("content", 1);
    ASSERT_EQ(MessageType::DOCUMENT_SYNC, docSyncMsg.type);
    
    auto ackMsg = MessageFactory::createOperationAckMessage("op_001", 1);
    ASSERT_EQ(MessageType::OPERATION_ACK, ackMsg.type);
}

// 测试15: 消息往返序列化
TEST(MessageTest, RoundTripSerialization) {
    Message original = MessageFactory::createOperationMessage(
        "op_test", "user_test", 42, "replace", 100, "Test content"
    );
    
    std::string json = original.toJson();
    Message deserialized = Message::fromJson(json);
    
    ASSERT_EQ(original.type, deserialized.type);
    ASSERT_EQ(original.messageId, deserialized.messageId);
    ASSERT_EQ(original.timestamp, deserialized.timestamp);
    ASSERT_EQ(original.payload.dump(), deserialized.payload.dump());
}

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Running Network Tests" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    test::TestRunner::getInstance().runAll();
    return 0;
}

#include "test_framework.h"
#include "utils/Config.h"
#include <fstream>

using namespace utils;

// 测试1: 基本配置加载
TEST(ConfigTest, BasicLoadFromFile) {
    auto& config = Config::getInstance();
    config.reset();
    
    // 创建测试配置文件
    std::string testConfig = "/tmp/test_config.json";
    std::ofstream file(testConfig);
    file << R"({
        "server": {
            "host": "127.0.0.1",
            "port": 9090
        }
    })";
    file.close();
    
    bool loaded = config.loadFromFile(testConfig);
    ASSERT_TRUE(loaded);
    
    ASSERT_STREQ("127.0.0.1", config.getServerHost().c_str());
    ASSERT_EQ(9090, config.getServerPort());
    
    std::remove(testConfig.c_str());
}

// 测试2: 从字符串加载配置
TEST(ConfigTest, LoadFromString) {
    auto& config = Config::getInstance();
    config.reset();
    
    std::string jsonStr = R"({
        "database": {
            "path": "test.db",
            "pool_size": 10
        }
    })";
    
    bool loaded = config.loadFromString(jsonStr);
    ASSERT_TRUE(loaded);
    
    ASSERT_STREQ("test.db", config.getDatabasePath().c_str());
    ASSERT_EQ(10, config.getDatabasePoolSize());
}

// 测试3: 默认值
TEST(ConfigTest, DefaultValues) {
    auto& config = Config::getInstance();
    config.reset();
    
    // 不加载任何配置，应该返回默认值
    ASSERT_STREQ("0.0.0.0", config.getServerHost().c_str());
    ASSERT_EQ(8080, config.getServerPort());
    ASSERT_EQ(1000, config.getMaxConnections());
    ASSERT_STREQ("collab_engine.db", config.getDatabasePath().c_str());
}

// 测试4: 保存配置到文件
TEST(ConfigTest, SaveToFile) {
    auto& config = Config::getInstance();
    config.reset();
    
    std::string testConfig = "/tmp/test_save_config.json";
    
    // 设置一些值
    config.set("server.host", "localhost");
    config.set("server.port", 3000);
    config.set("custom.key", "value");
    
    bool saved = config.saveToFile(testConfig);
    ASSERT_TRUE(saved);
    
    // 验证文件存在
    std::ifstream file(testConfig);
    ASSERT_TRUE(file.is_open());
    file.close();
    
    // 重新加载并验证 - 使用单例
    auto& config2 = Config::getInstance();
    config2.reset();
    bool loaded = config2.loadFromFile(testConfig);
    ASSERT_TRUE(loaded);
    
    ASSERT_STREQ("localhost", config2.getString("server.host", "").c_str());
    ASSERT_EQ(3000, config2.getInt("server.port", 0));
    ASSERT_STREQ("value", config2.getString("custom.key", "").c_str());
    
    std::remove(testConfig.c_str());
}

// 测试5: 嵌套配置访问
TEST(ConfigTest, NestedConfigAccess) {
    auto& config = Config::getInstance();
    config.reset();
    
    std::string jsonStr = R"({
        "level1": {
            "level2": {
                "level3": "deep_value"
            }
        }
    })";
    
    config.loadFromString(jsonStr);
    
    ASSERT_STREQ("deep_value", config.get<std::string>("level1.level2.level3", "").c_str());
}

// 测试6: 键存在检查
TEST(ConfigTest, KeyExists) {
    auto& config = Config::getInstance();
    config.reset();
    
    std::string jsonStr = R"({
        "existing_key": "value",
        "nested": {
            "key": "nested_value"
        }
    })";
    
    config.loadFromString(jsonStr);
    
    ASSERT_TRUE(config.hasKey("existing_key"));
    ASSERT_TRUE(config.hasKey("nested.key"));
    ASSERT_FALSE(config.hasKey("nonexistent_key"));
    ASSERT_FALSE(config.hasKey("nested.nonexistent"));
}

// 测试7: 错误处理 - 无效JSON
TEST(ConfigTest, InvalidJsonHandling) {
    auto& config = Config::getInstance();
    config.reset();
    
    std::string invalidJson = "{ invalid json }";
    
    bool loaded = config.loadFromString(invalidJson);
    ASSERT_FALSE(loaded);
    
    ASSERT_NOT_EMPTY(config.getLastError().c_str());
}

// 测试8: 错误处理 - 文件不存在
TEST(ConfigTest, FileNotFound) {
    auto& config = Config::getInstance();
    config.reset();
    
    bool loaded = config.loadFromFile("/nonexistent/path/config.json");
    ASSERT_FALSE(loaded);
    
    ASSERT_NOT_EMPTY(config.getLastError().c_str());
}

// 测试9: 各种数据类型
TEST(ConfigTest, VariousDataTypes) {
    auto& config = Config::getInstance();
    config.reset();
    
    std::string jsonStr = R"({
        "string_val": "hello",
        "int_val": 42,
        "double_val": 3.14,
        "bool_val": true
    })";
    
    config.loadFromString(jsonStr);
    
    ASSERT_STREQ("hello", config.get<std::string>("string_val", "").c_str());
    ASSERT_EQ(42, config.get<int>("int_val", 0));
    ASSERT_GT(3.15, config.get<double>("double_val", 0.0));
    ASSERT_LT(3.13, config.get<double>("double_val", 0.0));
    ASSERT_TRUE(config.get<bool>("bool_val", false));
}

// 测试10: 配置重置
TEST(ConfigTest, ConfigReset) {
    auto& config = Config::getInstance();
    config.reset();
    
    config.set("key1", "value1");
    config.set("key2", "value2");
    
    ASSERT_TRUE(config.hasKey("key1"));
    ASSERT_TRUE(config.hasKey("key2"));
    
    config.reset();
    
    ASSERT_FALSE(config.hasKey("key1"));
    ASSERT_FALSE(config.hasKey("key2"));
}

// 测试11: toJSON输出
TEST(ConfigTest, ToJsonOutput) {
    auto& config = Config::getInstance();
    config.reset();
    
    config.set("test.key", "value");
    
    std::string json = config.toJson();
    ASSERT_NOT_EMPTY(json.c_str());
    ASSERT_TRUE(json.find("test") != std::string::npos);
    ASSERT_TRUE(json.find("value") != std::string::npos);
}

// 测试12: 单例模式
TEST(ConfigTest, SingletonPattern) {
    auto& config1 = Config::getInstance();
    auto& config2 = Config::getInstance();
    
    ASSERT_EQ(&config1, &config2);
}

// 测试13: 完整配置示例
TEST(ConfigTest, FullConfigExample) {
    auto& config = Config::getInstance();
    config.reset();
    
    std::string fullConfig = R"({
        "server": {
            "host": "0.0.0.0",
            "port": 8080,
            "max_connections": 1000,
            "heartbeat_interval": 30
        },
        "database": {
            "path": "collab_engine.db",
            "pool_size": 5
        },
        "room": {
            "max_users_per_room": 50,
            "default_room_capacity": 10
        },
        "session": {
            "timeout_minutes": 30,
            "token_secret": "secret-key"
        },
        "logging": {
            "level": "INFO",
            "file_path": "logs/server.log"
        }
    })";
    
    bool loaded = config.loadFromString(fullConfig);
    ASSERT_TRUE(loaded);
    
    // 验证所有配置项
    ASSERT_STREQ("0.0.0.0", config.getServerHost().c_str());
    ASSERT_EQ(8080, config.getServerPort());
    ASSERT_EQ(1000, config.getMaxConnections());
    ASSERT_EQ(30, config.getHeartbeatInterval());
    
    ASSERT_STREQ("collab_engine.db", config.getDatabasePath().c_str());
    ASSERT_EQ(5, config.getDatabasePoolSize());
    
    ASSERT_EQ(50, config.getMaxUsersPerRoom());
    ASSERT_EQ(10, config.getDefaultRoomCapacity());
    
    ASSERT_EQ(30, config.getSessionTimeoutMinutes());
    ASSERT_STREQ("secret-key", config.getTokenSecret().c_str());
    
    ASSERT_STREQ("INFO", config.getLogLevel().c_str());
    ASSERT_STREQ("logs/server.log", config.getLogFilePath().c_str());
}

int main() {
    test::TestRunner::getInstance().runAll();
    return 0;
}

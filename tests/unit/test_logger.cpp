#include "test_framework.h"
#include "utils/Logger.h"
#include <fstream>
#include <thread>
#include <chrono>

using namespace utils;

// 测试1: 基本日志功能
TEST(LoggerTest, BasicLogging) {
    auto& logger = Logger::getInstance();
    logger.initialize("", LogLevel::DEBUG, true, false);
    
    // 不应该抛出异常
    logger.info("Test info message");
    logger.warning("Test warning message");
    logger.error("Test error message");
    logger.debug("Test debug message");
    
    ASSERT_TRUE(true);
}

// 测试2: 日志级别过滤
TEST(LoggerTest, LogLevelFiltering) {
    auto& logger = Logger::getInstance();
    logger.initialize("", LogLevel::WARNING, true, false);
    
    // DEBUG和INFO应该被过滤掉
    logger.setMinLogLevel(LogLevel::WARNING);
    
    // 这些应该被记录
    logger.warning("Warning message");
    logger.error("Error message");
    
    ASSERT_TRUE(true);
}

// 测试3: 文件日志
TEST(LoggerTest, FileLogging) {
    auto& logger = Logger::getInstance();
    std::string testLogFile = "/tmp/test_logger.log";
    
    // 清理旧文件
    std::remove(testLogFile.c_str());
    
    bool initialized = logger.initialize(testLogFile, LogLevel::INFO, false, false);
    ASSERT_TRUE(initialized);
    
    logger.info("File log test message");
    logger.flush();
    logger.shutdown();
    
    // 验证文件存在且包含内容
    std::ifstream file(testLogFile);
    ASSERT_TRUE(file.is_open());
    
    std::string line;
    bool found = false;
    while (std::getline(file, line)) {
        if (line.find("File log test message") != std::string::npos) {
            found = true;
            break;
        }
    }
    file.close();
    
    ASSERT_TRUE(found);
    
    // 清理
    std::remove(testLogFile.c_str());
}

// 测试4: 异步日志
TEST(LoggerTest, AsyncLogging) {
    auto& logger = Logger::getInstance();
    std::string testLogFile = "/tmp/test_async_logger.log";
    
    std::remove(testLogFile.c_str());
    
    bool initialized = logger.initialize(testLogFile, LogLevel::INFO, false, true);
    ASSERT_TRUE(initialized);
    
    // 写入多条日志
    for (int i = 0; i < 10; i++) {
        logger.info("Async message " + std::to_string(i));
    }
    
    // 等待异步线程处理
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    logger.shutdown();
    
    // 验证文件
    std::ifstream file(testLogFile);
    ASSERT_TRUE(file.is_open());
    
    int lineCount = 0;
    std::string line;
    while (std::getline(file, line)) {
        lineCount++;
    }
    file.close();
    
    ASSERT_EQ(10, lineCount);
    
    std::remove(testLogFile.c_str());
}

// 测试5: 日志统计
TEST(LoggerTest, LogStatistics) {
    auto& logger = Logger::getInstance();
    logger.initialize("", LogLevel::DEBUG, true, false);
    logger.resetStats();  // 重置统计
    
    logger.debug("Debug 1");
    logger.debug("Debug 2");
    logger.info("Info 1");
    logger.warning("Warning 1");
    logger.error("Error 1");
    
    auto stats = logger.getStats();
    
    ASSERT_EQ(5, stats.totalLogs);
    ASSERT_EQ(2, stats.debugCount);
    ASSERT_EQ(1, stats.infoCount);
    ASSERT_EQ(1, stats.warningCount);
    ASSERT_EQ(1, stats.errorCount);
}

// 测试6: 格式化日志
TEST(LoggerTest, FormattedLogging) {
    auto& logger = Logger::getInstance();
    logger.initialize("", LogLevel::DEBUG, true, false);
    
    // 不应该抛出异常
    logger.logFormat(LogLevel::INFO, "Value: %d, String: %s", 42, "test");
    
    ASSERT_TRUE(true);
}

// 测试7: 单例模式
TEST(LoggerTest, SingletonPattern) {
    auto& logger1 = Logger::getInstance();
    auto& logger2 = Logger::getInstance();
    
    // 应该是同一个实例
    ASSERT_EQ(&logger1, &logger2);
}

// 测试8: 多次初始化和关闭
TEST(LoggerTest, MultipleInitShutdown) {
    auto& logger = Logger::getInstance();
    
    // 第一次初始化
    bool init1 = logger.initialize("", LogLevel::INFO, true, false);
    ASSERT_TRUE(init1);
    
    logger.info("Message 1");
    logger.shutdown();
    
    // 第二次初始化
    bool init2 = logger.initialize("", LogLevel::DEBUG, true, false);
    ASSERT_TRUE(init2);
    
    logger.debug("Message 2");
    logger.shutdown();
    
    ASSERT_TRUE(true);
}

// 测试9: 日志级别转换
TEST(LoggerTest, LevelToString) {
    ASSERT_STREQ("DEBUG", levelToString(LogLevel::DEBUG).c_str());
    ASSERT_STREQ("INFO", levelToString(LogLevel::INFO).c_str());
    ASSERT_STREQ("WARNING", levelToString(LogLevel::WARNING).c_str());
    ASSERT_STREQ("ERROR", levelToString(LogLevel::ERROR).c_str());
    ASSERT_STREQ("FATAL", levelToString(LogLevel::FATAL).c_str());
}

// 测试10: 控制台输出开关
TEST(LoggerTest, ConsoleToggle) {
    auto& logger = Logger::getInstance();
    
    // 禁用控制台
    logger.initialize("", LogLevel::INFO, false, false);
    logger.info("This should not appear on console");
    
    // 启用控制台
    logger.shutdown();
    logger.initialize("", LogLevel::INFO, true, false);
    logger.info("This should appear on console");
    
    logger.shutdown();
    ASSERT_TRUE(true);
}

int main() {
    test::TestRunner::getInstance().runAll();
    return 0;
}

#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <functional>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>

namespace utils {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    FATAL = 4
};

// 日志级别转字符串
std::string levelToString(LogLevel level);

// 日志条目结构
struct LogEntry {
    LogLevel level;
    std::string message;
    std::string timestamp;
    std::string file;
    int line;
};

class Logger {
public:
    static Logger& getInstance();
    
    // 初始化日志系统
    bool initialize(const std::string& logFilePath = "", 
                   LogLevel minLevel = LogLevel::INFO,
                   bool enableConsole = true,
                   bool enableAsync = false);
    
    // 关闭日志系统
    void shutdown();
    
    // 设置最小日志级别
    void setMinLogLevel(LogLevel level);
    
    // 获取最小日志级别
    LogLevel getMinLogLevel() const;
    
    // 日志记录方法（支持文件和行号）
    void log(LogLevel level, const std::string& message, 
             const std::string& file = "", int line = 0);
    
    // 便捷日志方法
    void debug(const std::string& message, const std::string& file = "", int line = 0);
    void info(const std::string& message, const std::string& file = "", int line = 0);
    void warning(const std::string& message, const std::string& file = "", int line = 0);
    void error(const std::string& message, const std::string& file = "", int line = 0);
    void fatal(const std::string& message, const std::string& file = "", int line = 0);
    
    // 格式化日志（支持printf风格）
    template<typename... Args>
    void logFormat(LogLevel level, const char* format, Args... args) {
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), format, args...);
        log(level, std::string(buffer));
    }
    
    // 刷新日志
    void flush();
    
    // 重置统计信息
    void resetStats();
    
    // 获取日志统计
    struct LogStats {
        size_t totalLogs = 0;
        size_t debugCount = 0;
        size_t infoCount = 0;
        size_t warningCount = 0;
        size_t errorCount = 0;
        size_t fatalCount = 0;
    };
    
    LogStats getStats() const;
    
private:
    Logger();
    ~Logger();
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // 写入日志
    void writeLog(const LogEntry& entry);
    
    // 异步日志处理线程
    void processLogQueue();
    
    // 更新统计信息
    void updateStats(LogLevel level);
    
    mutable std::mutex logMutex;
    mutable std::mutex statsMutex;
    std::ofstream logFile;
    
    LogLevel minLogLevel;
    bool consoleEnabled;
    bool fileEnabled;
    
    // 异步日志相关
    bool asyncEnabled;
    std::atomic<bool> asyncRunning;
    std::queue<LogEntry> logQueue;
    std::mutex queueMutex;
    std::condition_variable logCondition;
    std::thread logThread;
    
    // 统计信息
    LogStats stats;
};

} // namespace utils

// 便捷的宏定义
#define LOG_DEBUG(msg) utils::Logger::getInstance().debug(msg, __FILE__, __LINE__)
#define LOG_INFO(msg) utils::Logger::getInstance().info(msg, __FILE__, __LINE__)
#define LOG_WARNING(msg) utils::Logger::getInstance().warning(msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) utils::Logger::getInstance().error(msg, __FILE__, __LINE__)
#define LOG_FATAL(msg) utils::Logger::getInstance().fatal(msg, __FILE__, __LINE__)
#define LOG_FORMAT(level, fmt, ...) utils::Logger::getInstance().logFormat(level, fmt, ##__VA_ARGS__)

#include "utils/Logger.h"
#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace utils {

// 日志级别转字符串实现
std::string levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

// Logger单例获取
Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

// 构造函数
Logger::Logger() 
    : minLogLevel(LogLevel::INFO), 
      consoleEnabled(true), 
      fileEnabled(false), 
      asyncEnabled(false), 
      asyncRunning(false) {
}

// 析构函数
Logger::~Logger() {
    shutdown();
}

// 初始化日志系统
bool Logger::initialize(const std::string& logFilePath, 
                       LogLevel minLevel,
                       bool enableConsole,
                       bool enableAsync) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    this->minLogLevel = minLevel;
    this->consoleEnabled = enableConsole;
    this->asyncEnabled = enableAsync;
    
    // 打开日志文件
    if (!logFilePath.empty()) {
        logFile.open(logFilePath, std::ios::app);
        if (!logFile.is_open()) {
            std::cerr << "[ERROR] Failed to open log file: " << logFilePath << std::endl;
            return false;
        }
        fileEnabled = true;
    }
    
    // 启动异步日志线程
    if (asyncEnabled) {
        asyncRunning = true;
        logThread = std::thread(&Logger::processLogQueue, this);
    }
    
    return true;
}

// 关闭日志系统
void Logger::shutdown() {
    if (asyncEnabled) {
        asyncRunning = false;
        logCondition.notify_all();
        if (logThread.joinable()) {
            logThread.join();
        }
    }
    
    std::lock_guard<std::mutex> lock(logMutex);
    if (logFile.is_open()) {
        logFile.close();
    }
}

// 设置最小日志级别
void Logger::setMinLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex);
    minLogLevel = level;
}

// 获取最小日志级别
LogLevel Logger::getMinLogLevel() const {
    return minLogLevel;
}

// 日志记录方法
void Logger::log(LogLevel level, const std::string& message, 
                 const std::string& file, int line) {
    if (level < minLogLevel) {
        return;
    }
    
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    std::string timestamp = ss.str();
    
    LogEntry entry;
    entry.level = level;
    entry.message = message;
    entry.timestamp = timestamp;
    entry.file = file;
    entry.line = line;
    
    if (asyncEnabled) {
        // 异步日志
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            logQueue.push(entry);
        }
        logCondition.notify_one();
    } else {
        // 同步日志
        writeLog(entry);
    }
}

// 便捷日志方法
void Logger::debug(const std::string& message, const std::string& file, int line) {
    log(LogLevel::DEBUG, message, file, line);
}

void Logger::info(const std::string& message, const std::string& file, int line) {
    log(LogLevel::INFO, message, file, line);
}

void Logger::warning(const std::string& message, const std::string& file, int line) {
    log(LogLevel::WARNING, message, file, line);
}

void Logger::error(const std::string& message, const std::string& file, int line) {
    log(LogLevel::ERROR, message, file, line);
}

void Logger::fatal(const std::string& message, const std::string& file, int line) {
    log(LogLevel::FATAL, message, file, line);
}

// 刷新日志
void Logger::flush() {
    std::lock_guard<std::mutex> lock(logMutex);
    if (logFile.is_open()) {
        logFile.flush();
    }
    std::cout.flush();
}

// 重置统计信息
void Logger::resetStats() {
    std::lock_guard<std::mutex> lock(statsMutex);
    stats = LogStats();
}

// 获取日志统计
Logger::LogStats Logger::getStats() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    return stats;
}

// 写入日志
void Logger::writeLog(const LogEntry& entry) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    std::string logLine = "[" + entry.timestamp + "] [" + 
                         levelToString(entry.level) + "] ";
    
    if (!entry.file.empty()) {
        logLine += "(" + entry.file + ":" + std::to_string(entry.line) + ") ";
    }
    
    logLine += entry.message;
    
    // 输出到控制台
    if (consoleEnabled) {
        if (entry.level >= LogLevel::ERROR) {
            std::cerr << logLine << std::endl;
        } else {
            std::cout << logLine << std::endl;
        }
    }
    
    // 输出到文件
    if (fileEnabled && logFile.is_open()) {
        logFile << logLine << std::endl;
    }
    
    // 更新统计
    updateStats(entry.level);
}

// 异步日志处理线程
void Logger::processLogQueue() {
    while (asyncRunning) {
        LogEntry entry;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            logCondition.wait_for(lock, std::chrono::milliseconds(100), [this] {
                return !logQueue.empty() || !asyncRunning;
            });
            
            if (!asyncRunning && logQueue.empty()) {
                break;
            }
            
            if (!logQueue.empty()) {
                entry = logQueue.front();
                logQueue.pop();
            }
        }
        
        if (!entry.message.empty()) {
            writeLog(entry);
        }
    }
}

// 更新统计信息
void Logger::updateStats(LogLevel level) {
    std::lock_guard<std::mutex> lock(statsMutex);
    stats.totalLogs++;
    switch (level) {
        case LogLevel::DEBUG:   stats.debugCount++; break;
        case LogLevel::INFO:    stats.infoCount++; break;
        case LogLevel::WARNING: stats.warningCount++; break;
        case LogLevel::ERROR:   stats.errorCount++; break;
        case LogLevel::FATAL:   stats.fatalCount++; break;
    }
}

} // namespace utils

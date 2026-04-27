#pragma once

#include <string>
#include <cstdint>

namespace utils {

/**
 * 日志轮转管理器
 */
class LogRotator {
public:
    /**
     * 构造函数
     * @param logDir 日志目录
     * @param maxFileSizeMB 单个日志文件最大大小（MB）
     * @param maxBackupCount 保留的备份文件数量
     */
    LogRotator(const std::string& logDir, 
               uint64_t maxFileSizeMB = 10,
               int maxBackupCount = 5);
    
    /**
     * 检查并执行日志轮转
     * @return 是否执行了轮转
     */
    bool rotateIfNeeded();
    
    /**
     * 手动触发日志轮转
     */
    void rotate();
    
    /**
     * 清理旧日志文件
     */
    void cleanupOldLogs();
    
private:
    /**
     * 获取当前日志文件大小（字节）
     */
    uint64_t getCurrentLogSize();
    
    /**
     * 重命名日志文件
     */
    void renameLogFile(const std::string& oldPath, const std::string& newPath);
    
    std::string logDir;
    std::string currentLogFile;
    uint64_t maxFileSizeBytes;
    int maxBackupCount;
};

} // namespace utils

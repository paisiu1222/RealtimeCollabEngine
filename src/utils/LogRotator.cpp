#include "utils/LogRotator.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <ctime>
#include <iomanip>

namespace utils {

LogRotator::LogRotator(const std::string& logDir, 
                       uint64_t maxFileSizeMB,
                       int maxBackupCount)
    : logDir(logDir),
      currentLogFile(logDir + "/server.log"),
      maxFileSizeBytes(maxFileSizeMB * 1024 * 1024),
      maxBackupCount(maxBackupCount) {
}

bool LogRotator::rotateIfNeeded() {
    uint64_t currentSize = getCurrentLogSize();
    
    if (currentSize >= maxFileSizeBytes) {
        rotate();
        return true;
    }
    
    return false;
}

void LogRotator::rotate() {
    // 生成时间戳
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    std::string timestamp = oss.str();
    
    // 构建备份文件名
    std::string backupFile = logDir + "/server_" + timestamp + ".log";
    
    // 重命名当前日志文件
    renameLogFile(currentLogFile, backupFile);
    
    // 清理旧日志
    cleanupOldLogs();
}

void LogRotator::cleanupOldLogs() {
    // 简化实现：实际应该扫描目录并删除最旧的备份
    // 这里仅作为示例，生产环境需要更完善的实现
}

uint64_t LogRotator::getCurrentLogSize() {
    struct stat st;
    if (stat(currentLogFile.c_str(), &st) == 0) {
        return static_cast<uint64_t>(st.st_size);
    }
    return 0;
}

void LogRotator::renameLogFile(const std::string& oldPath, const std::string& newPath) {
    std::rename(oldPath.c_str(), newPath.c_str());
}

} // namespace utils

#pragma once

#include <string>
#include <atomic>
#include <vector>
#include <mutex>
#include <chrono>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace utils {

/**
 * 性能指标收集器
 */
class Metrics {
public:
    /**
     * 获取单例实例
     */
    static Metrics& getInstance();
    
    /**
     * 记录操作
     */
    void recordOperation();
    
    /**
     * 记录延迟（毫秒）
     */
    void recordLatency(uint64_t latencyMs);
    
    /**
     * 增加活跃连接数
     */
    void incrementActiveConnections();
    
    /**
     * 减少活跃连接数
     */
    void decrementActiveConnections();
    
    /**
     * 记录错误
     */
    void recordError();
    
    /**
     * 获取指标JSON
     */
    std::string getMetricsJson();
    
    /**
     * 重置所有指标
     */
    void reset();
    
    /**
     * 获取启动时间
     */
    uint64_t getUptimeSeconds() const;
    
private:
    Metrics();
    
    std::atomic<uint64_t> totalOperations{0};
    std::atomic<uint64_t> activeConnections{0};
    std::atomic<uint64_t> totalErrors{0};
    
    std::vector<uint64_t> latencies;
    std::mutex latenciesMutex;
    
    std::chrono::steady_clock::time_point startTime;
};

} // namespace utils

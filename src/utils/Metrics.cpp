#include "utils/Metrics.h"
#include <algorithm>
#include <numeric>

namespace utils {

Metrics& Metrics::getInstance() {
    static Metrics instance;
    return instance;
}

Metrics::Metrics() : startTime(std::chrono::steady_clock::now()) {
}

void Metrics::recordOperation() {
    totalOperations.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::recordLatency(uint64_t latencyMs) {
    std::lock_guard<std::mutex> lock(latenciesMutex);
    latencies.push_back(latencyMs);
    
    // 保留最近1000个延迟记录
    if (latencies.size() > 1000) {
        latencies.erase(latencies.begin(), latencies.begin() + 100);
    }
}

void Metrics::incrementActiveConnections() {
    activeConnections.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::decrementActiveConnections() {
    uint64_t current = activeConnections.load(std::memory_order_relaxed);
    if (current > 0) {
        activeConnections.fetch_sub(1, std::memory_order_relaxed);
    }
}

void Metrics::recordError() {
    totalErrors.fetch_add(1, std::memory_order_relaxed);
}

std::string Metrics::getMetricsJson() {
    json metrics;
    
    metrics["uptime_seconds"] = getUptimeSeconds();
    metrics["total_operations"] = totalOperations.load();
    metrics["active_connections"] = activeConnections.load();
    metrics["total_errors"] = totalErrors.load();
    
    // 计算延迟统计
    std::lock_guard<std::mutex> lock(latenciesMutex);
    if (!latencies.empty()) {
        uint64_t sum = std::accumulate(latencies.begin(), latencies.end(), 0ULL);
        uint64_t avg = sum / latencies.size();
        
        // 排序以计算百分位数
        std::vector<uint64_t> sorted_latencies = latencies;
        std::sort(sorted_latencies.begin(), sorted_latencies.end());
        
        uint64_t p50 = sorted_latencies[sorted_latencies.size() * 50 / 100];
        uint64_t p95 = sorted_latencies[sorted_latencies.size() * 95 / 100];
        uint64_t p99 = sorted_latencies[sorted_latencies.size() * 99 / 100];
        uint64_t min_latency = sorted_latencies.front();
        uint64_t max_latency = sorted_latencies.back();
        
        metrics["latency"] = {
            {"avg_ms", avg},
            {"p50_ms", p50},
            {"p95_ms", p95},
            {"p99_ms", p99},
            {"min_ms", min_latency},
            {"max_ms", max_latency},
            {"count", latencies.size()}
        };
    } else {
        metrics["latency"] = {
            {"avg_ms", 0},
            {"p50_ms", 0},
            {"p95_ms", 0},
            {"p99_ms", 0},
            {"min_ms", 0},
            {"max_ms", 0},
            {"count", 0}
        };
    }
    
    return metrics.dump(2);
}

void Metrics::reset() {
    totalOperations.store(0);
    activeConnections.store(0);
    totalErrors.store(0);
    
    std::lock_guard<std::mutex> lock(latenciesMutex);
    latencies.clear();
    
    startTime = std::chrono::steady_clock::now();
}

uint64_t Metrics::getUptimeSeconds() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);
    return duration.count();
}

} // namespace utils

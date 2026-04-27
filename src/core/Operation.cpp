#include "core/Operation.h"
#include <uuid/uuid.h>
#include <sstream>
#include <iomanip>

namespace core {

// ==================== Operation 实现 ====================

Operation::Operation() 
    : version(0), type(OperationType::RETAIN), position(0) {
    timestamp = getCurrentTimestamp();
}

Operation::Operation(const std::string& opId, const std::string& userId, 
                     uint64_t version, OperationType type, 
                     size_t position, const std::string& content)
    : opId(opId), userId(userId), version(version), 
      type(type), position(position), content(content) {
    timestamp = getCurrentTimestamp();
}

std::string Operation::toJson() const {
    nlohmann::json json;
    json["opId"] = opId;
    json["userId"] = userId;
    json["version"] = version;
    json["type"] = operationTypeToString(type);
    json["position"] = position;
    json["content"] = content;
    
    // 序列化时间戳为ISO8601格式
    auto time_t_ts = std::chrono::system_clock::to_time_t(timestamp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t_ts), "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    json["timestamp"] = ss.str();
    
    return json.dump();
}

Operation Operation::fromJson(const std::string& json) {
    Operation op;
    try {
        auto j = nlohmann::json::parse(json);
        
        op.opId = j.value("opId", "");
        op.userId = j.value("userId", "");
        op.version = j.value("version", 0);
        op.type = stringToOperationType(j.value("type", "retain"));
        op.position = j.value("position", 0);
        op.content = j.value("content", "");
        
        // 解析时间戳（简化处理，实际生产环境需要更健壮的解析）
        op.timestamp = getCurrentTimestamp();
        
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to parse operation: ") + e.what());
    }
    
    return op;
}

std::string Operation::typeToString(OperationType type) {
    return operationTypeToString(type);
}

OperationType Operation::stringToType(const std::string& str) {
    return stringToOperationType(str);
}

std::chrono::system_clock::time_point Operation::getCurrentTimestamp() {
    return std::chrono::system_clock::now();
}

// ==================== 辅助函数实现 ====================

std::string operationTypeToString(OperationType type) {
    switch (type) {
        case OperationType::INSERT: return "insert";
        case OperationType::DELETE: return "delete";
        case OperationType::REPLACE: return "replace";
        case OperationType::RETAIN: return "retain";
        default: return "unknown";
    }
}

OperationType stringToOperationType(const std::string& str) {
    if (str == "insert") return OperationType::INSERT;
    if (str == "delete") return OperationType::DELETE;
    if (str == "replace") return OperationType::REPLACE;
    if (str == "retain") return OperationType::RETAIN;
    throw std::runtime_error("Unknown operation type: " + str);
}

} // namespace core

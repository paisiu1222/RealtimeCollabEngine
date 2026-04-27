#ifndef OPERATION_H
#define OPERATION_H

#include <string>
#include <cstdint>
#include <chrono>
#include <nlohmann/json.hpp>

namespace core {

/**
 * 操作类型枚举
 */
enum class OperationType {
    INSERT,     // 插入文本
    DELETE,     // 删除文本
    REPLACE,    // 替换文本
    RETAIN      // 保留（无操作，用于对齐）
};

/**
 * 操作结果枚举
 */
enum class OperationResult {
    SUCCESS,            // 成功
    VERSION_CONFLICT,   // 版本冲突
    INVALID_OPERATION,  // 无效操作
    POSITION_OUT_OF_RANGE // 位置越界
};

/**
 * 操作结构体
 * 表示一个编辑操作
 */
struct Operation {
    std::string opId;                                         // 操作唯一ID
    std::string userId;                                       // 用户ID
    uint64_t version;                                         // 版本号
    OperationType type;                                       // 操作类型
    size_t position;                                          // 操作位置
    std::string content;                                      // 操作内容（插入/替换的文本，或删除的文本）
    std::chrono::system_clock::time_point timestamp;          // 时间戳
    
    /**
     * 默认构造函数
     */
    Operation();
    
    /**
     * 构造函数
     */
    Operation(const std::string& opId, const std::string& userId, 
              uint64_t version, OperationType type, 
              size_t position, const std::string& content);
    
    /**
     * 将操作序列化为JSON字符串
     */
    std::string toJson() const;
    
    /**
     * 从JSON字符串解析操作
     */
    static Operation fromJson(const std::string& json);
    
    /**
     * 获取操作类型的字符串表示
     */
    static std::string typeToString(OperationType type);
    
    /**
     * 从字符串解析操作类型
     */
    static OperationType stringToType(const std::string& str);
    
    /**
     * 获取当前时间戳
     */
    static std::chrono::system_clock::time_point getCurrentTimestamp();
};

/**
 * 辅助函数：将OperationType转换为字符串
 */
std::string operationTypeToString(OperationType type);

/**
 * 辅助函数：从字符串解析OperationType
 */
OperationType stringToOperationType(const std::string& str);

} // namespace core

#endif // OPERATION_H

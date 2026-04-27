#include "core/ConflictResolver.h"
#include "core/OTAlgorithm.h"
#include <algorithm>

namespace core {

// ==================== 公共方法 ====================

ConflictResolver::ConflictType ConflictResolver::detectConflict(
    const Operation& localOp, 
    const Operation& remoteOp
) {
    // 同一用户的操作不冲突
    if (localOp.userId == remoteOp.userId) {
        return ConflictType::NONE;
    }
    
    // 检查版本冲突
    auto versionConflict = checkVersionConflict(localOp.version, remoteOp.version);
    if (versionConflict != ConflictType::NONE) {
        return versionConflict;
    }
    
    // 检查位置冲突
    return checkPositionConflict(localOp, remoteOp);
}

Operation ConflictResolver::resolveConflict(
    const Operation& localOp, 
    const Operation& remoteOp
) {
    // 使用时间戳优先策略：最后写入获胜
    // 如果本地操作时间戳更新，则转换远程操作
    if (localOp.timestamp > remoteOp.timestamp) {
        // 本地优先，返回转换后的本地操作
        return OTAlgorithm::transform(localOp, remoteOp);
    } else {
        // 远程优先，返回转换后的远程操作
        return OTAlgorithm::transform(remoteOp, localOp);
    }
}

std::string ConflictResolver::conflictTypeToString(ConflictType type) {
    switch (type) {
        case ConflictType::NONE:
            return "NONE";
        case ConflictType::VERSION_CONFLICT:
            return "VERSION_CONFLICT";
        case ConflictType::POSITION_CONFLICT:
            return "POSITION_CONFLICT";
        default:
            return "UNKNOWN";
    }
}

// ==================== 私有方法 ====================

ConflictResolver::ConflictType ConflictResolver::checkVersionConflict(
    uint64_t localVersion, 
    uint64_t remoteVersion
) {
    if (localVersion != remoteVersion) {
        return ConflictType::VERSION_CONFLICT;
    }
    return ConflictType::NONE;
}

ConflictResolver::ConflictType ConflictResolver::checkPositionConflict(
    const Operation& op1, 
    const Operation& op2
) {
    // 如果操作类型都是RETAIN，不冲突
    if (op1.type == OperationType::RETAIN && op2.type == OperationType::RETAIN) {
        return ConflictType::NONE;
    }
    
    // 计算操作影响的范围
    size_t op1Start = op1.position;
    size_t op1End = op1.position + (op1.type == OperationType::DELETE ? op1.content.length() : 0);
    
    size_t op2Start = op2.position;
    size_t op2End = op2.position + (op2.type == OperationType::DELETE ? op2.content.length() : 0);
    
    // 对于INSERT操作，影响范围就是插入位置
    if (op1.type == OperationType::INSERT) {
        op1End = op1Start + 1;
    }
    if (op2.type == OperationType::INSERT) {
        op2End = op2Start + 1;
    }
    
    // 检查是否有重叠
    if (op1Start < op2End && op2Start < op1End) {
        return ConflictType::POSITION_CONFLICT;
    }
    
    return ConflictType::NONE;
}

} // namespace core

#ifndef CONFLICT_RESOLVER_H
#define CONFLICT_RESOLVER_H

#include "core/Operation.h"
#include <string>

namespace core {

/**
 * 冲突解决器
 * 检测和解决并发操作冲突
 */
class ConflictResolver {
public:
    /**
     * 冲突类型枚举
     */
    enum class ConflictType {
        NONE,               // 无冲突
        VERSION_CONFLICT,   // 版本冲突
        POSITION_CONFLICT   // 位置冲突
    };
    
    /**
     * 检测两个操作之间的冲突
     * @param localOp 本地操作
     * @param remoteOp 远程操作
     * @return 冲突类型
     */
    static ConflictType detectConflict(const Operation& localOp, const Operation& remoteOp);
    
    /**
     * 解决冲突并返回转换后的操作
     * @param localOp 本地操作
     * @param remoteOp 远程操作
     * @return 解决冲突后的操作
     */
    static Operation resolveConflict(const Operation& localOp, const Operation& remoteOp);
    
    /**
     * 将冲突类型转换为字符串
     */
    static std::string conflictTypeToString(ConflictType type);

private:
    /**
     * 检查版本冲突
     */
    static ConflictType checkVersionConflict(uint64_t localVersion, uint64_t remoteVersion);
    
    /**
     * 检查位置冲突
     */
    static ConflictType checkPositionConflict(const Operation& op1, const Operation& op2);
};

} // namespace core

#endif // CONFLICT_RESOLVER_H

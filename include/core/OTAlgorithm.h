#ifndef OT_ALGORITHM_H
#define OT_ALGORITHM_H

#include "core/Operation.h"
#include <string>
#include <functional>
#include <map>

namespace core {

/**
 * OT算法类
 * 实现Operational Transformation算法，用于解决并发编辑冲突
 * 
 * 采用策略模式替代大量switch-case，提高代码可维护性和扩展性
 */
class OTAlgorithm {
public:
    /**
     * 转换两个并发操作
     * @param localOp 本地操作（将要应用的）
     * @param remoteOp 远程操作（已经应用的）
     * @return 转换后的本地操作
     */
    static Operation transform(const Operation& localOp, const Operation& remoteOp);
    
    /**
     * 检查两个操作是否冲突
     */
    static bool isConflict(const Operation& op1, const Operation& op2);
    
    /**
     * 合并多个操作为一个复合操作（优化用）
     */
    static Operation mergeOperations(const std::vector<Operation>& operations);

private:
    // ==================== 转换函数类型定义 ====================
    using TransformFunc = std::function<Operation(const Operation&, const Operation&)>;
    
    /**
     * 初始化转换函数表
     * 使用静态局部变量确保只初始化一次
     */
    static const std::map<std::pair<OperationType, OperationType>, TransformFunc>& getTransformTable();
    
    // ==================== 具体转换策略 ====================
    
    /** INSERT vs INSERT */
    static Operation transformInsertInsert(const Operation& local, const Operation& remote);
    
    /** INSERT vs DELETE */
    static Operation transformInsertDelete(const Operation& local, const Operation& remote);
    
    /** DELETE vs INSERT */
    static Operation transformDeleteInsert(const Operation& local, const Operation& remote);
    
    /** DELETE vs DELETE */
    static Operation transformDeleteDelete(const Operation& local, const Operation& remote);
    
    /** INSERT vs REPLACE */
    static Operation transformInsertReplace(const Operation& local, const Operation& remote);
    
    /** DELETE vs REPLACE */
    static Operation transformDeleteReplace(const Operation& local, const Operation& remote);
    
    /** REPLACE vs INSERT */
    static Operation transformReplaceInsert(const Operation& local, const Operation& remote);
    
    /** REPLACE vs DELETE */
    static Operation transformReplaceDelete(const Operation& local, const Operation& remote);
    
    /** REPLACE vs REPLACE */
    static Operation transformReplaceReplace(const Operation& local, const Operation& remote);
    
    /** 默认转换（无需转换的情况） */
    static Operation defaultTransform(const Operation& local, const Operation& /*remote*/);
};

} // namespace core

#endif // OT_ALGORITHM_H

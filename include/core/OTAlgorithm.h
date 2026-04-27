#ifndef OT_ALGORITHM_H
#define OT_ALGORITHM_H

#include "core/Operation.h"
#include <string>

namespace core {

/**
 * OT算法类
 * 实现Operational Transformation算法，用于解决并发编辑冲突
 */
class OTAlgorithm {
public:
    /**
     * 转换两个并发操作
     * @param localOp 本地操作（将要应用的）
     * @param remoteOp 远程操作（已经应用的）
     * @return 转换后的本地操作
     * 
     * 转换原则：
     * - 如果remoteOp在localOp之前插入/删除，需要调整localOp的位置
     * - 保证两个操作应用后的最终状态一致
     */
    static Operation transform(const Operation& localOp, const Operation& remoteOp);
    
    /**
     * 检查两个操作是否冲突
     * @param op1 操作1
     * @param op2 操作2
     * @return 是否冲突
     */
    static bool isConflict(const Operation& op1, const Operation& op2);
    
    /**
     * 合并多个操作为一个复合操作（优化用）
     * @param operations 操作列表
     * @return 合并后的操作（如果无法合并则返回空）
     */
    static Operation mergeOperations(const std::vector<Operation>& operations);

private:
    // ==================== 内部转换方法 ====================
    
    /**
     * INSERT vs INSERT 转换
     */
    static Operation transformInsertInsert(const Operation& local, const Operation& remote);
    
    /**
     * INSERT vs DELETE 转换
     */
    static Operation transformInsertDelete(const Operation& local, const Operation& remote);
    
    /**
     * DELETE vs INSERT 转换
     */
    static Operation transformDeleteInsert(const Operation& local, const Operation& remote);
    
    /**
     * DELETE vs DELETE 转换
     */
    static Operation transformDeleteDelete(const Operation& local, const Operation& remote);
    
    /**
     * INSERT vs REPLACE 转换
     */
    static Operation transformInsertReplace(const Operation& local, const Operation& remote);
    
    /**
     * DELETE vs REPLACE 转换
     */
    static Operation transformDeleteReplace(const Operation& local, const Operation& remote);
    
    /**
     * REPLACE vs INSERT 转换
     */
    static Operation transformReplaceInsert(const Operation& local, const Operation& remote);
    
    /**
     * REPLACE vs DELETE 转换
     */
    static Operation transformReplaceDelete(const Operation& local, const Operation& remote);
    
    /**
     * REPLACE vs REPLACE 转换
     */
    static Operation transformReplaceReplace(const Operation& local, const Operation& remote);
};

} // namespace core

#endif // OT_ALGORITHM_H

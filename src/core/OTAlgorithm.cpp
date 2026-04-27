#include "core/OTAlgorithm.h"
#include <algorithm>

namespace core {

// ==================== 公共方法 ====================

Operation OTAlgorithm::transform(const Operation& localOp, const Operation& remoteOp) {
    // 如果操作来自同一用户或版本号不同，不需要转换
    if (localOp.userId == remoteOp.userId || localOp.version != remoteOp.version) {
        return localOp;
    }
    
    // 根据操作类型分发到不同的转换函数
    switch (localOp.type) {
        case OperationType::INSERT:
            switch (remoteOp.type) {
                case OperationType::INSERT:
                    return transformInsertInsert(localOp, remoteOp);
                case OperationType::DELETE:
                    return transformInsertDelete(localOp, remoteOp);
                case OperationType::REPLACE:
                    return transformInsertReplace(localOp, remoteOp);
                default:
                    return localOp;
            }
            
        case OperationType::DELETE:
            switch (remoteOp.type) {
                case OperationType::INSERT:
                    return transformDeleteInsert(localOp, remoteOp);
                case OperationType::DELETE:
                    return transformDeleteDelete(localOp, remoteOp);
                case OperationType::REPLACE:
                    return transformDeleteReplace(localOp, remoteOp);
                default:
                    return localOp;
            }
            
        case OperationType::REPLACE:
            switch (remoteOp.type) {
                case OperationType::INSERT:
                    return transformReplaceInsert(localOp, remoteOp);
                case OperationType::DELETE:
                    return transformReplaceDelete(localOp, remoteOp);
                case OperationType::REPLACE:
                    return transformReplaceReplace(localOp, remoteOp);
                default:
                    return localOp;
            }
            
        default:
            return localOp;
    }
}

bool OTAlgorithm::isConflict(const Operation& op1, const Operation& op2) {
    // 如果操作不重叠，则不冲突
    if (op1.position + op1.content.length() <= op2.position ||
        op2.position + op2.content.length() <= op1.position) {
        return false;
    }
    
    // 同一用户的操作不视为冲突
    if (op1.userId == op2.userId) {
        return false;
    }
    
    return true;
}

Operation OTAlgorithm::mergeOperations(const std::vector<Operation>& operations) {
    if (operations.empty()) {
        return Operation();
    }
    
    if (operations.size() == 1) {
        return operations[0];
    }
    
    // 简化实现：只合并在同一位置的连续INSERT操作
    // 实际生产环境需要更复杂的合并逻辑
    
    Operation merged = operations[0];
    for (size_t i = 1; i < operations.size(); ++i) {
        const auto& op = operations[i];
        
        // 只能合并相同类型的操作
        if (op.type != merged.type) {
            return Operation(); // 无法合并
        }
        
        // 对于INSERT操作，如果在相同位置，可以合并内容
        if (op.type == OperationType::INSERT && op.position == merged.position) {
            merged.content += op.content;
        } else {
            return Operation(); // 无法合并
        }
    }
    
    return merged;
}

// ==================== 内部转换方法 ====================

Operation OTAlgorithm::transformInsertInsert(const Operation& local, const Operation& remote) {
    Operation result = local;
    
    // 如果本地插入位置在远程插入位置之前或相同，无需调整
    if (local.position <= remote.position) {
        return result;
    }
    
    // 否则，本地插入位置需要后移（跳过远程插入的内容）
    result.position += remote.content.length();
    
    return result;
}

Operation OTAlgorithm::transformInsertDelete(const Operation& local, const Operation& remote) {
    Operation result = local;
    
    // 如果本地插入位置在远程删除区域之前，无需调整
    if (local.position <= remote.position) {
        return result;
    }
    
    // 如果本地插入位置在远程删除区域内，调整到删除区域的起始位置
    if (local.position < remote.position + remote.content.length()) {
        result.position = remote.position;
        return result;
    }
    
    // 如果本地插入位置在远程删除区域之后，前移（减去删除的长度）
    result.position -= remote.content.length();
    
    return result;
}

Operation OTAlgorithm::transformDeleteInsert(const Operation& local, const Operation& remote) {
    Operation result = local;
    
    // 如果本地删除位置在远程插入位置之前，无需调整
    if (local.position + local.content.length() <= remote.position) {
        return result;
    }
    
    // 如果本地删除位置在远程插入位置之后，后移（加上插入的长度）
    if (local.position >= remote.position) {
        result.position += remote.content.length();
        return result;
    }
    
    // 如果本地删除与远程插入重叠，需要分割删除操作
    // 简化处理：保持原位置，但可能需要调整删除长度
    size_t overlapStart = std::max(local.position, remote.position);
    size_t overlapEnd = std::min(local.position + local.content.length(), 
                                 remote.position + remote.content.length());
    
    if (overlapEnd > overlapStart) {
        // 有重叠，调整删除长度
        result.content = local.content.substr(0, overlapStart - local.position);
        // 注意：这里简化处理，实际应该保留被删除的内容信息
    }
    
    return result;
}

Operation OTAlgorithm::transformDeleteDelete(const Operation& local, const Operation& remote) {
    Operation result = local;
    
    // 如果本地删除在远程删除之前，无需调整
    if (local.position + local.content.length() <= remote.position) {
        return result;
    }
    
    // 如果本地删除在远程删除之后，前移
    if (local.position >= remote.position + remote.content.length()) {
        result.position -= remote.content.length();
        return result;
    }
    
    // 如果有重叠，需要计算剩余需要删除的部分
    size_t localStart = local.position;
    size_t localEnd = local.position + local.content.length();
    size_t remoteStart = remote.position;
    size_t remoteEnd = remote.position + remote.content.length();
    
    // 计算重叠部分
    size_t overlapStart = std::max(localStart, remoteStart);
    size_t overlapEnd = std::min(localEnd, remoteEnd);
    
    if (overlapStart >= overlapEnd) {
        // 没有重叠，只需调整位置
        if (localStart > remoteStart) {
            result.position -= remote.content.length();
        }
    } else {
        // 有重叠，调整删除范围和位置
        // 简化处理：如果完全被覆盖，变为空操作
        if (localStart >= remoteStart && localEnd <= remoteEnd) {
            result.type = OperationType::RETAIN;
            result.content = "";
        } else {
            // 部分重叠，调整位置
            if (localStart < remoteStart) {
                result.content = local.content.substr(0, remoteStart - localStart);
            } else {
                result.position = remoteStart;
                result.content = local.content.substr(remoteEnd - localStart);
            }
        }
    }
    
    return result;
}

Operation OTAlgorithm::transformInsertReplace(const Operation& local, const Operation& remote) {
    // REPLACE可以看作是先DELETE再INSERT
    // 简化处理：将REPLACE视为DELETE来处理位置调整
    Operation deleteOp = remote;
    deleteOp.type = OperationType::DELETE;
    
    return transformInsertDelete(local, deleteOp);
}

Operation OTAlgorithm::transformDeleteReplace(const Operation& local, const Operation& remote) {
    // 类似INSERT vs REPLACE的处理
    Operation deleteOp = remote;
    deleteOp.type = OperationType::DELETE;
    
    return transformDeleteDelete(local, deleteOp);
}

Operation OTAlgorithm::transformReplaceInsert(const Operation& local, const Operation& remote) {
    Operation result = local;
    
    // 如果替换位置在插入位置之前，无需调整
    if (local.position + local.content.length() <= remote.position) {
        return result;
    }
    
    // 如果替换位置在插入位置之后，后移
    if (local.position >= remote.position) {
        result.position += remote.content.length();
        return result;
    }
    
    // 如果有重叠，保持原位（简化处理）
    return result;
}

Operation OTAlgorithm::transformReplaceDelete(const Operation& local, const Operation& remote) {
    Operation result = local;
    
    // 如果替换位置在删除位置之前，无需调整
    if (local.position + local.content.length() <= remote.position) {
        return result;
    }
    
    // 如果替换位置在删除位置之后，前移
    if (local.position >= remote.position + remote.content.length()) {
        result.position -= remote.content.length();
        return result;
    }
    
    // 如果有重叠，简化处理：保持原位
    return result;
}

Operation OTAlgorithm::transformReplaceReplace(const Operation& local, const Operation& remote) {
    Operation result = local;
    
    // 如果两个替换操作不重叠，无需调整
    if (local.position + local.content.length() <= remote.position ||
        remote.position + remote.content.length() <= local.position) {
        return result;
    }
    
    // 如果完全重叠，保持原位（后面的操作会覆盖前面的）
    // 如果部分重叠，简化处理：保持原位
    return result;
}

} // namespace core

#include "core/DocumentState.h"
#include <algorithm>

namespace core {

// ==================== 构造函数 ====================

DocumentState::DocumentState(const std::string& docId)
    : docId(docId), currentVersion(0), logger(utils::Logger::getInstance()) {
    logger.info("DocumentState created for document: " + docId);
}

// ==================== 核心方法 ====================

OperationResult DocumentState::applyOperation(const Operation& op) {
    std::lock_guard<std::mutex> lock(stateMutex);
    
    // 验证版本号
    if (op.version != currentVersion + 1) {
        logger.warning("Version conflict for doc " + docId + 
                      ": expected " + std::to_string(currentVersion + 1) + 
                      ", got " + std::to_string(op.version));
        return OperationResult::VERSION_CONFLICT;
    }
    
    // 验证操作位置
    if (!validatePosition(op)) {
        logger.error("Invalid position in operation: " + std::to_string(op.position));
        return OperationResult::POSITION_OUT_OF_RANGE;
    }
    
    // 根据操作类型执行
    bool success = false;
    switch (op.type) {
        case OperationType::INSERT:
            success = executeInsert(op);
            break;
        case OperationType::DELETE:
            success = executeDelete(op);
            break;
        case OperationType::REPLACE:
            success = executeReplace(op);
            break;
        case OperationType::RETAIN:
            // RETAIN操作不改变内容，直接成功
            success = true;
            break;
        default:
            logger.error("Unknown operation type");
            return OperationResult::INVALID_OPERATION;
    }
    
    if (!success) {
        return OperationResult::INVALID_OPERATION;
    }
    
    // 更新版本号和历史记录
    currentVersion++;
    history.push_back(op);
    
    // 限制历史记录大小
    if (history.size() > MAX_HISTORY_SIZE) {
        history.pop_front();
    }
    
    logger.debug("Operation applied to doc " + docId + 
                ": version=" + std::to_string(currentVersion) + 
                ", type=" + operationTypeToString(op.type));
    
    return OperationResult::SUCCESS;
}

std::string DocumentState::getContent() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return content;
}

void DocumentState::setContent(const std::string& newContent) {
    std::lock_guard<std::mutex> lock(stateMutex);
    content = newContent;
    logger.info("Document content set for doc " + docId +
               ", length=" + std::to_string(content.length()));
}

void DocumentState::setVersion(uint64_t version) {
    std::lock_guard<std::mutex> lock(stateMutex);
    currentVersion = version;
}

uint64_t DocumentState::getVersion() const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return currentVersion;
}

std::vector<Operation> DocumentState::getHistory(int count) const {
    std::lock_guard<std::mutex> lock(stateMutex);
    
    if (count <= 0 || static_cast<size_t>(count) >= history.size()) {
        return std::vector<Operation>(history.begin(), history.end());
    }
    
    // 返回最近的count个操作
    auto start = history.end() - count;
    return std::vector<Operation>(start, history.end());
}

bool DocumentState::rollbackToVersion(uint64_t targetVersion) {
    std::lock_guard<std::mutex> lock(stateMutex);
    
    if (targetVersion >= currentVersion) {
        logger.warning("Cannot rollback to version >= current version");
        return false;
    }
    
    if (targetVersion < currentVersion - history.size()) {
        logger.error("Target version not in history");
        return false;
    }

    // 从空内容开始重放到目标版本
    content.clear();
    for (const auto& op : history) {
        if (op.version > targetVersion) break;
        switch (op.type) {
            case OperationType::INSERT:
                content.insert(op.position, op.content);
                break;
            case OperationType::DELETE:
                if (op.position + op.content.length() <= content.length())
                    content.erase(op.position, op.content.length());
                break;
            case OperationType::REPLACE:
                if (op.position + op.content.length() <= content.length())
                    content.replace(op.position, op.content.length(), op.content);
                break;
            default: break;
        }
    }

    currentVersion = targetVersion;

    logger.info("Rolled back to version " + std::to_string(targetVersion));
    return true;
}

bool DocumentState::canApplyVersion(uint64_t version) const {
    std::lock_guard<std::mutex> lock(stateMutex);
    return version == currentVersion + 1;
}

// ==================== 内部方法 ====================

bool DocumentState::executeInsert(const Operation& op) {
    try {
        content.insert(op.position, op.content);
        return true;
    } catch (const std::exception& e) {
        logger.error("Failed to execute insert: " + std::string(e.what()));
        return false;
    }
}

bool DocumentState::executeDelete(const Operation& op) {
    try {
        if (op.position + op.content.length() > content.length()) {
            logger.error("Delete operation out of bounds");
            return false;
        }
        content.erase(op.position, op.content.length());
        return true;
    } catch (const std::exception& e) {
        logger.error("Failed to execute delete: " + std::string(e.what()));
        return false;
    }
}

bool DocumentState::executeReplace(const Operation& op) {
    try {
        if (op.position + op.content.length() > content.length()) {
            logger.error("Replace operation out of bounds");
            return false;
        }
        content.replace(op.position, op.content.length(), op.content);
        return true;
    } catch (const std::exception& e) {
        logger.error("Failed to execute replace: " + std::string(e.what()));
        return false;
    }
}

bool DocumentState::validatePosition(const Operation& op) const {
    if (currentVersion == 0 && content.empty()) {
        return op.position == 0;
    }

    if (op.type == OperationType::INSERT) {
        return op.position <= content.length();
    }
    return op.position + op.content.length() <= content.length();
}

} // namespace core

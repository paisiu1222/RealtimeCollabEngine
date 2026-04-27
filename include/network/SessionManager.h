#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <string>
#include <map>
#include <memory>
#include <mutex>
#include "network/Room.h"
#include "utils/Logger.h"

namespace network {

/**
 * 会话管理器
 * 管理所有房间和用户的会话状态
 */
class SessionManager {
public:
    /**
     * 获取单例实例
     */
    static SessionManager& getInstance();
    
    /**
     * 创建或获取房间
     * @param roomId 房间ID
     * @param docId 文档ID
     * @return 房间指针
     */
    std::shared_ptr<Room> getOrCreateRoom(const std::string& roomId, const std::string& docId);
    
    /**
     * 获取房间
     * @param roomId 房间ID
     * @return 房间指针，如果不存在返回nullptr
     */
    std::shared_ptr<Room> getRoom(const std::string& roomId) const;
    
    /**
     * 删除房间
     * @param roomId 房间ID
     * @return 是否成功
     */
    bool removeRoom(const std::string& roomId);
    
    /**
     * 添加用户到房间
     * @param roomId 房间ID
     * @param userId 用户ID
     * @param connectionId WebSocket连接ID
     * @return 是否成功
     */
    bool addUserToRoom(
        const std::string& roomId, 
        const std::string& userId, 
        const std::string& connectionId
    );
    
    /**
     * 从房间移除用户
     * @param roomId 房间ID
     * @param userId 用户ID
     * @return 是否成功
     */
    bool removeUserFromRoom(const std::string& roomId, const std::string& userId);
    
    /**
     * 获取用户所在的房间ID
     * @param userId 用户ID
     * @return 房间ID，如果不在任何房间返回空字符串
     */
    std::string getUserRoom(const std::string& userId) const;
    
    /**
     * 获取活跃房间数量
     */
    size_t getActiveRoomCount() const;
    
    /**
     * 获取所有房间ID列表
     */
    std::vector<std::string> getAllRoomIds() const;
    
    /**
     * 清理空房间
     */
    void cleanupEmptyRooms();

private:
    SessionManager();
    ~SessionManager() = default;
    
    // 禁止拷贝和赋值
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;
    
    // ==================== 成员变量 ====================
    
    mutable std::mutex managerMutex;                          // 管理器互斥锁
    std::map<std::string, std::shared_ptr<Room>> rooms_;      // roomId -> Room映射
    std::map<std::string, std::string> userRooms_;            // userId -> roomId映射
    
    utils::Logger& logger;                                    // 日志记录器
};

} // namespace network

#endif // SESSION_MANAGER_H

#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <mutex>
#include <map>
#include <optional>

namespace utils {

class Config {
public:
    static Config& getInstance();
    
    // 从文件加载配置
    bool loadFromFile(const std::string& path);
    
    // 从字符串加载配置
    bool loadFromString(const std::string& jsonStr);
    
    // 保存配置到文件
    bool saveToFile(const std::string& path = "");
    
    // 服务器配置
    std::string getServerHost() const;
    int getServerPort() const;
    int getMaxConnections() const;
    int getHeartbeatInterval() const;
    
    // 数据库配置
    std::string getDatabasePath() const;
    int getDatabasePoolSize() const;
    
    // 房间配置
    int getMaxUsersPerRoom() const;
    int getDefaultRoomCapacity() const;
    
    // 会话配置
    int getSessionTimeoutMinutes() const;
    std::string getTokenSecret() const;
    
    // 日志配置
    std::string getLogLevel() const;
    std::string getLogFilePath() const;
    
    // 通用获取方法
    template<typename T>
    T get(const std::string& key, const T& defaultValue) const {
        std::lock_guard<std::mutex> lock(configMutex);
        
        try {
            return getValue<T>(configData, key, defaultValue);
        } catch (...) {
            return defaultValue;
        }
    }
    
    // 设置配置值
    template<typename T>
    void set(const std::string& key, const T& value) {
        std::lock_guard<std::mutex> lock(configMutex);
        setValue(configData, key, value);
    }
    
    // 检查键是否存在
    bool hasKey(const std::string& key) const;
    
    // 获取最后的错误信息
    std::string getLastError() const;
    
    // 获取配置的JSON字符串
    std::string toJson(int indent = 2) const;
    
    // 重置配置
    void reset();
    
    // 便捷方法（公开）
    std::string getString(const std::string& key, const std::string& defaultValue) const;
    int getInt(const std::string& key, int defaultValue) const;
    double getDouble(const std::string& key, double defaultValue) const;
    bool getBool(const std::string& key, bool defaultValue) const;
    
private:
    Config();
    ~Config();
    
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    
    // 递归获取嵌套值
    template<typename T>
    T getValue(const nlohmann::json& json, const std::string& key, const T& defaultValue) const {
        std::istringstream iss(key);
        std::string part;
        
        nlohmann::json current = json;
        while (std::getline(iss, part, '.')) {
            if (!current.contains(part)) {
                return defaultValue;
            }
            current = current[part];
        }
        
        return current.get<T>();
    }
    
    // 递归设置嵌套值
    template<typename T>
    void setValue(nlohmann::json& json, const std::string& key, const T& value) {
        std::istringstream iss(key);
        std::string part;
        
        nlohmann::json* current = &json;
        std::vector<std::string> parts;
        
        while (std::getline(iss, part, '.')) {
            parts.push_back(part);
        }
        
        for (size_t i = 0; i < parts.size() - 1; ++i) {
            if (!current->contains(parts[i]) || !(*current)[parts[i]].is_object()) {
                (*current)[parts[i]] = nlohmann::json::object();
            }
            current = &(*current)[parts[i]];
        }
        
        (*current)[parts.back()] = value;
    }
    
    // 检查键是否存在（内部实现）
    bool hasKeyInternal(const nlohmann::json& json, const std::string& key) const;
    
    mutable std::mutex configMutex;
    nlohmann::json configData;
    std::string configFilePath;
    std::string lastError;
};

} // namespace utils

#include "utils/Config.h"
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <mutex>

namespace utils {

// Config单例获取
Config& Config::getInstance() {
    static Config instance;
    return instance;
}

// 构造函数
Config::Config() {
}

// 析构函数
Config::~Config() {
}

// 从文件加载配置
bool Config::loadFromFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(configMutex);
    
    std::ifstream file(path);
    if (!file.is_open()) {
        lastError = "Failed to open config file: " + path;
        return false;
    }
    
    try {
        file >> configData;
        configFilePath = path;
        lastError.clear();
        return true;
    } catch (const nlohmann::json::exception& e) {
        lastError = std::string("JSON parse error: ") + e.what();
        return false;
    } catch (const std::exception& e) {
        lastError = std::string("Error loading config: ") + e.what();
        return false;
    }
}

// 从字符串加载配置
bool Config::loadFromString(const std::string& jsonStr) {
    std::lock_guard<std::mutex> lock(configMutex);
    
    try {
        configData = nlohmann::json::parse(jsonStr);
        lastError.clear();
        return true;
    } catch (const nlohmann::json::exception& e) {
        lastError = std::string("JSON parse error: ") + e.what();
        return false;
    }
}

// 保存配置到文件
bool Config::saveToFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(configMutex);
    
    std::string savePath = path.empty() ? configFilePath : path;
    if (savePath.empty()) {
        lastError = "No config file path specified";
        return false;
    }
    
    try {
        std::ofstream file(savePath);
        if (!file.is_open()) {
            lastError = "Failed to open file for writing: " + savePath;
            return false;
        }
        
        file << configData.dump(4); // 格式化输出，缩进4空格
        file.close();
        lastError.clear();
        return true;
    } catch (const std::exception& e) {
        lastError = std::string("Error saving config: ") + e.what();
        return false;
    }
}

// 服务器配置
std::string Config::getServerHost() const {
    return getString("server.host", "0.0.0.0");
}

int Config::getServerPort() const {
    return getInt("server.port", 8080);
}

int Config::getMaxConnections() const {
    return getInt("server.max_connections", 1000);
}

int Config::getHeartbeatInterval() const {
    return getInt("server.heartbeat_interval", 30);
}

// 数据库配置
std::string Config::getDatabasePath() const {
    return getString("database.path", "collab_engine.db");
}

int Config::getDatabasePoolSize() const {
    return getInt("database.pool_size", 5);
}

// 房间配置
int Config::getMaxUsersPerRoom() const {
    return getInt("room.max_users_per_room", 50);
}

int Config::getDefaultRoomCapacity() const {
    return getInt("room.default_room_capacity", 10);
}

// 会话配置
int Config::getSessionTimeoutMinutes() const {
    return getInt("session.timeout_minutes", 30);
}

std::string Config::getTokenSecret() const {
    return getString("session.token_secret", "default-secret-key-change-in-production");
}

// 日志配置
std::string Config::getLogLevel() const {
    return getString("logging.level", "INFO");
}

std::string Config::getLogFilePath() const {
    return getString("logging.file_path", "logs/server.log");
}

// 检查键是否存在
bool Config::hasKey(const std::string& key) const {
    std::lock_guard<std::mutex> lock(configMutex);
    return hasKeyInternal(configData, key);
}

// 获取最后的错误信息
std::string Config::getLastError() const {
    std::lock_guard<std::mutex> lock(configMutex);
    return lastError;
}

// 获取配置的JSON字符串
std::string Config::toJson(int indent) const {
    std::lock_guard<std::mutex> lock(configMutex);
    return configData.dump(indent);
}

// 重置配置
void Config::reset() {
    std::lock_guard<std::mutex> lock(configMutex);
    configData.clear();
    configFilePath.clear();
    lastError.clear();
}

// 便捷方法实现
std::string Config::getString(const std::string& key, const std::string& defaultValue) const {
    return get<std::string>(key, defaultValue);
}

int Config::getInt(const std::string& key, int defaultValue) const {
    return get<int>(key, defaultValue);
}

double Config::getDouble(const std::string& key, double defaultValue) const {
    return get<double>(key, defaultValue);
}

bool Config::getBool(const std::string& key, bool defaultValue) const {
    return get<bool>(key, defaultValue);
}

// 检查键是否存在（内部实现）
bool Config::hasKeyInternal(const nlohmann::json& json, const std::string& key) const {
    std::istringstream iss(key);
    std::string part;
    
    nlohmann::json current = json;
    while (std::getline(iss, part, '.')) {
        if (!current.contains(part)) {
            return false;
        }
        current = current[part];
    }
    
    return true;
}

} // namespace utils

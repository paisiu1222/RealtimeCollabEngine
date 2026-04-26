# 实时协作编辑系统 - 项目开发周期文档

## 一、项目总体规划

### 1.1 项目信息
- **项目名称**: Realtime-Collab-Engine (实时协作编辑引擎)
- **开发周期**: 8周 (约2个月)
- **工作模式**: 单人开发，每周40小时
- **技术栈**: C++14 + Boost.Asio + SQLite3 + Crow + nlohmann/json

### 1.2 里程碑概览
```
Milestone 1 (Week 2): 基础设施完成
Milestone 2 (Week 4): 核心功能完成
Milestone 3 (Week 6): 高级功能完成
Milestone 4 (Week 8): 项目交付
```

---

## 二、详细开发流程

### 阶段1: 基础设施搭建 (Week 1-2)

#### Week 1: 项目骨架与核心依赖

**Day 1-2: 项目初始化**

**任务清单**:
- [ ] 创建CMakeLists.txt构建配置
- [ ] 设计目录结构
- [ ] 集成第三方库 (Boost.Asio, nlohmann/json, Crow, SQLite3)
- [ ] 编写README和项目文档

**交付物**:
- 可编译的项目骨架
- 完整的目录结构
- README.md文档

**验收标准**:
- `cmake ..` 和 `make` 能成功编译
- 所有依赖库正确链接

---

**Day 3-4: 日志与配置模块**

**任务清单**:
- [ ] 实现Logger单例（线程安全）
- [ ] 实现Config解析器（JSON配置）
- [ ] 编写单元测试

**关键代码**:
```cpp
// Logger.h - 线程安全的日志单例
class Logger {
public:
    static Logger& getInstance();
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    
private:
    Logger();
    std::mutex logMutex;
    std::ofstream logFile;
};

// Config.h - JSON配置解析
class Config {
public:
    bool loadFromFile(const std::string& path);
    std::string getServerHost() const;
    int getServerPort() const;
    int getMaxConnections() const;
    
private:
    nlohmann::json configData;
};
```

**交付物**:
- Logger模块完整实现
- Config模块完整实现
- 单元测试用例（至少5个）

**验收标准**:
- 日志能正确写入文件
- 配置文件能正确解析
- 单元测试通过率100%

---

**Day 5-7: 数据库层**

**任务清单**:
- [ ] 设计SQLite3 schema
- [ ] 实现Database连接池
- [ ] 实现DAO层（DocumentDAO, OperationDAO）
- [ ] 编写集成测试

**关键代码**:
```cpp
// Database.h - 数据库连接管理
class Database {
public:
    bool initialize(const std::string& dbPath);
    sqlite3* getConnection();
    void releaseConnection(sqlite3* conn);
    bool executeQuery(const std::string& sql);
    
private:
    std::string dbPath;
    std::vector<sqlite3*> connectionPool;
    std::mutex poolMutex;
};

// DocumentDAO.h - 文档数据访问
class DocumentDAO {
public:
    bool insertDocument(const Document& doc);
    Document getDocument(const std::string& docId);
    bool updateDocument(const Document& doc);
    bool deleteDocument(const std::string& docId);
    
private:
    Database& db;
};
```

**SQL Schema**:
```sql
CREATE TABLE documents (
    doc_id TEXT PRIMARY KEY,
    title TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP,
    current_version INTEGER DEFAULT 0,
    owner_id TEXT
);

CREATE TABLE operations (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    doc_id TEXT NOT NULL,
    op_id TEXT UNIQUE NOT NULL,
    user_id TEXT NOT NULL,
    version INTEGER NOT NULL,
    op_type TEXT NOT NULL,
    position INTEGER,
    content TEXT,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(doc_id) REFERENCES documents(doc_id)
);

CREATE INDEX idx_ops_doc_version ON operations(doc_id, version);
```

**交付物**:
- 数据库初始化脚本
- DocumentDAO和OperationDAO实现
- 集成测试用例

**验收标准**:
- 数据库表正确创建
- CRUD操作正常工作
- 索引优化生效

---

#### Week 2: 网络层基础

**Day 1-3: WebSocket服务器**

**任务清单**:
- [ ] 基于Boost.Asio实现WebSocket服务端
- [ ] 连接管理与心跳机制
- [ ] 消息编解码（JSON格式）

**关键代码**:
```cpp
// WebSocketServer.h
class WebSocketServer {
public:
    WebSocketServer(const std::string& host, int port);
    void start();
    void stop();
    void broadcastMessage(const std::string& roomId, const std::string& message);
    
private:
    void acceptConnection();
    void handleMessage(WebSocketConnection& conn, const std::string& message);
    void sendHeartbeat(WebSocketConnection& conn);
    
    boost::asio::io_context ioContext;
    tcp::acceptor acceptor;
    std::map<std::string, std::vector<WebSocketConnection>> roomConnections;
};
```

**消息协议**:
```json
{
  "type": "operation",
  "payload": {
    "opId": "uuid-123",
    "userId": "user-456",
    "version": 42,
    "operation": {
      "type": "INSERT",
      "position": 10,
      "content": "Hello"
    }
  }
}
```

**交付物**:
- WebSocket服务器原型
- 消息编解码器
- 心跳检测机制

**验收标准**:
- 客户端能成功连接
- 消息能正确收发
- 心跳检测正常工作

---

**Day 4-5: HTTP API框架**

**任务清单**:
- [ ] 集成Crow框架
- [ ] 实现基础路由
- [ ] CORS配置

**关键代码**:
```cpp
// HttpServer.h
class HttpServer {
public:
    HttpServer(int port);
    void start();
    void registerRoutes();
    
private:
    crow::SimpleApp app;
    void setupDocumentRoutes();
    void setupAuthRoutes();
    void setupRoomRoutes();
};

// 示例路由
CROW_ROUTE(app, "/api/documents").methods(crow::HTTPMethod::POST)
([](const crow::request& req) {
    auto body = nlohmann::json::parse(req.body);
    // 处理文档创建
    return crow::response(201, "Document created");
});
```

**交付物**:
- HTTP服务器框架
- 5个基础API端点
- CORS中间件

**验收标准**:
- API能正常响应
- CORS配置正确
- 错误处理完善

---

**Day 6-7: 联调与测试**

**任务清单**:
- [ ] WebSocket客户端测试工具
- [ ] API接口测试（Postman集合）
- [ ] 压力测试（100并发连接）

**测试工具**:
```javascript
// websocket-test-client.js
const WebSocket = require('ws');
const ws = new WebSocket('ws://localhost:8080');

ws.on('open', () => {
    console.log('Connected');
    ws.send(JSON.stringify({
        type: 'join_room',
        payload: { roomId: 'test-room' }
    }));
});

ws.on('message', (data) => {
    console.log('Received:', data.toString());
});
```

**交付物**:
- WebSocket测试客户端
- Postman API测试集合
- 压力测试报告

**验收标准**:
- 100并发连接稳定
- API响应时间 < 100ms
- 无内存泄漏

---

### 阶段1里程碑检查 (Week 2结束)

**Milestone 1 验收清单**:
- [ ] 项目可编译运行
- [ ] WebSocket连接正常
- [ ] 基础API可用
- [ ] 单元测试覆盖率 > 60%
- [ ] 数据库CRUD正常
- [ ] 日志系统工作正常

---

### 阶段2: 核心同步引擎 (Week 3-4)

#### Week 3: OT算法实现

**Day 1-2: 数据结构设计**

**任务清单**:
- [ ] Operation结构体定义
- [ ] DocumentState类设计
- [ ] 版本号管理机制

**关键代码**:
```cpp
// Operation.h
struct Operation {
    enum Type { INSERT, DELETE, REPLACE, FORMAT };
    
    std::string opId;
    std::string userId;
    uint64_t version;
    Type type;
    size_t position;
    std::string content;
    std::chrono::system_clock::time_point timestamp;
    
    std::string toJson() const;
    static Operation fromJson(const std::string& json);
};

// DocumentState.h
class DocumentState {
private:
    std::string content;
    uint64_t currentVersion;
    std::deque<Operation> history;
    std::mutex stateMutex;
    
public:
    OperationResult applyOperation(const Operation& op);
    std::string getContent() const;
    uint64_t getVersion() const;
    std::vector<Operation> getHistory(int count) const;
};
```

**交付物**:
- Operation结构体完整定义
- DocumentState类框架
- 版本号管理机制

**验收标准**:
- 数据结构设计合理
- 线程安全保证
- JSON序列化正常

---

**Day 3-5: OT算法核心**

**任务清单**:
- [ ] 实现INSERT-INSERT转换
- [ ] 实现INSERT-DELETE转换
- [ ] 实现DELETE-DELETE转换
- [ ] 编写算法单元测试（边界条件）

**关键代码**:
```cpp
// OTAlgorithm.h
class OTAlgorithm {
public:
    static Operation transform(const Operation& local, const Operation& remote);
    
private:
    static Operation transformInsertInsert(const Operation& local, const Operation& remote);
    static Operation transformInsertDelete(const Operation& local, const Operation& remote);
    static Operation transformDeleteDelete(const Operation& local, const Operation& remote);
};

// OTAlgorithm.cpp
Operation OTAlgorithm::transformInsertInsert(const Operation& local, const Operation& remote) {
    Operation result = local;
    if (local.position <= remote.position) {
        return result; // 无需调整
    } else {
        result.position += remote.content.length(); // 位置后移
        return result;
    }
}

Operation OTAlgorithm::transformInsertDelete(const Operation& local, const Operation& remote) {
    Operation result = local;
    if (local.position <= remote.position) {
        return result;
    } else {
        result.position -= remote.content.length(); // 位置前移
        return result;
    }
}
```

**测试用例**:
```cpp
// test_ot_algorithm.cpp
TEST(OTAlgorithm, TransformInsertInsert) {
    Operation op1{"op1", "user1", 1, Operation::INSERT, 5, "Hello"};
    Operation op2{"op2", "user2", 1, Operation::INSERT, 3, "World"};
    
    Operation transformed = OTAlgorithm::transform(op1, op2);
    EXPECT_EQ(transformed.position, 8); // 5 + 3
}

TEST(OTAlgorithm, TransformInsertDelete) {
    Operation op1{"op1", "user1", 1, Operation::INSERT, 10, "X"};
    Operation op2{"op2", "user2", 1, Operation::DELETE, 5, "Hello"};
    
    Operation transformed = OTAlgorithm::transform(op1, op2);
    EXPECT_EQ(transformed.position, 5); // 10 - 5
}
```

**交付物**:
- OT算法完整实现
- 单元测试用例（至少20个）
- 算法正确性验证报告

**验收标准**:
- 所有转换规则正确
- 边界条件覆盖完整
- 单元测试通过率100%

---

**Day 6-7: 操作应用逻辑**

**任务清单**:
- [ ] applyOperation方法实现
- [ ] 操作历史管理
- [ ] 版本回滚支持

**关键代码**:
```cpp
// DocumentState.cpp
OperationResult DocumentState::applyOperation(const Operation& op) {
    std::lock_guard<std::mutex> lock(stateMutex);
    
    if (op.version != currentVersion + 1) {
        return OperationResult::VERSION_CONFLICT;
    }
    
    switch (op.type) {
        case Operation::INSERT:
            content.insert(op.position, op.content);
            break;
        case Operation::DELETE:
            content.erase(op.position, op.content.length());
            break;
        // ... 其他操作类型
    }
    
    currentVersion++;
    history.push_back(op);
    
    // 限制历史记录大小
    if (history.size() > 1000) {
        history.pop_front();
    }
    
    return OperationResult::SUCCESS;
}
```

**交付物**:
- 操作应用逻辑完整实现
- 历史管理机制
- 版本回滚功能

**验收标准**:
- 操作能正确应用到文档
- 版本号递增正常
- 历史记录管理正确

---

#### Week 4: 同步协议与广播

**Day 1-3: 消息协议定义**

**任务清单**:
- [ ] 定义WebSocket消息格式
- [ ] 实现消息序列化/反序列化
- [ ] 操作ACK机制

**关键代码**:
```cpp
// MessageProtocol.h
struct Message {
    enum Type { OPERATION, ACK, ERROR, HEARTBEAT };
    
    Type type;
    std::string payload;
    
    std::string serialize() const;
    static Message deserialize(const std::string& json);
};

class MessageProtocol {
public:
    static std::string createOperationMessage(const Operation& op);
    static std::string createAckMessage(const std::string& opId);
    static std::string createErrorMessage(const std::string& errorMsg);
    
    static Operation parseOperationMessage(const std::string& json);
};
```

**交付物**:
- 完整的消息协议定义
- 序列化/反序列化工具
- ACK机制实现

**验收标准**:
- 消息格式统一
- 序列化效率良好
- ACK机制可靠

---

**Day 4-5: 房间管理**

**任务清单**:
- [ ] Room类实现
- [ ] 用户加入/离开逻辑
- [ ] 操作广播机制

**关键代码**:
```cpp
// Room.h
class Room {
private:
    std::string roomId;
    std::string docId;
    std::set<std::string> users;
    DocumentState document;
    std::mutex roomMutex;
    
public:
    bool addUser(const std::string& userId);
    bool removeUser(const std::string& userId);
    void broadcastOperation(const Operation& op, const std::string& senderId);
    DocumentState& getDocument();
};

// SessionManager.cpp
void SessionManager::broadcastToRoom(const std::string& roomId, const std::string& message) {
    auto room = getRoom(roomId);
    if (!room) return;
    
    for (const auto& userId : room->getUsers()) {
        auto conn = getUserConnection(userId);
        if (conn) {
            conn->sendMessage(message);
        }
    }
}
```

**交付物**:
- Room类完整实现
- 用户管理逻辑
- 广播机制

**验收标准**:
- 用户能正确加入/离开房间
- 广播消息送达所有用户
- 线程安全保证

---

**Day 6-7: 端到端测试**

**任务清单**:
- [ ] 双用户协作测试
- [ ] 冲突场景测试
- [ ] 性能测试（延迟、吞吐量）

**测试场景**:
```
场景1: 两个用户同时插入文本
  用户A在位置5插入"Hello"
  用户B在位置3插入"World"
  预期: 两个操作都正确应用，无冲突

场景2: 一个用户插入，另一个用户删除
  用户A在位置10插入"X"
  用户B删除位置5-9的文本
  预期: 插入位置自动调整

场景3: 高并发压力测试
  10个用户同时编辑
  每秒100个操作
  预期: 无数据丢失，延迟<100ms
```

**交付物**:
- 多用户协作演示
- 冲突场景测试报告
- 性能测试报告

**验收标准**:
- 双用户协作正常
- 冲突能自动解决
- P95延迟 < 100ms
- 吞吐量 > 500 ops/sec

---

### 阶段2里程碑检查 (Week 4结束)

**Milestone 2 验收清单**:
- [ ] OT算法正确实现
- [ ] 多用户协作演示成功
- [ ] 冲突自动解决
- [ ] 单元测试覆盖率 > 80%
- [ ] 性能指标达标
- [ ] 消息协议完整

---

### 阶段3: 高级功能 (Week 5-6)

#### Week 5: 冲突解决与离线支持

**Day 1-2: 冲突检测**

**任务清单**:
- [ ] 版本号冲突检测
- [ ] 并发操作识别

**关键代码**:
```cpp
// ConflictResolver.h
class ConflictResolver {
public:
    enum ConflictType { NONE, VERSION_CONFLICT, POSITION_CONFLICT };
    
    ConflictType detectConflict(const Operation& localOp, const Operation& remoteOp);
    Operation resolveConflict(const Operation& localOp, const Operation& remoteOp);
    
private:
    ConflictType checkVersionConflict(uint64_t localVersion, uint64_t remoteVersion);
    ConflictType checkPositionConflict(const Operation& op1, const Operation& op2);
};
```

**交付物**:
- 冲突检测模块
- 冲突类型定义

**验收标准**:
- 能准确检测各类冲突
- 误报率低

---

**Day 3-4: 自动合并**

**任务清单**:
- [ ] 时间戳优先策略
- [ ] 操作重排序

**关键代码**:
```cpp
// ConflictResolver.cpp
Operation ConflictResolver::resolveConflict(const Operation& localOp, const Operation& remoteOp) {
    // 时间戳优先：最后写入获胜
    if (localOp.timestamp > remoteOp.timestamp) {
        return transformAndApply(localOp, remoteOp);
    } else {
        return transformAndApply(remoteOp, localOp);
    }
}
```

**交付物**:
- 自动合并策略实现
- 操作重排序逻辑

**验收标准**:
- 冲突能自动解决
- 数据一致性保证

---

**Day 5-7: 离线编辑**

**任务清单**:
- [ ] 本地操作队列
- [ ] 批量同步逻辑
- [ ] 冲突提示API

**关键代码**:
```cpp
// OfflineQueue.h
class OfflineQueue {
private:
    std::queue<Operation> pendingOps;
    std::mutex queueMutex;
    
public:
    void enqueue(const Operation& op);
    std::vector<Operation> dequeueAll();
    bool isEmpty() const;
};

// SyncManager.h
class SyncManager {
public:
    void handleReconnect(const std::string& userId);
    void syncPendingOperations(const std::string& userId);
    
private:
    OfflineQueue offlineQueues;
};
```

**交付物**:
- 离线编辑支持
- 批量同步机制
- 冲突提示API

**验收标准**:
- 离线操作能缓存
- 重连后能同步
- 冲突能提示用户

---

#### Week 6: 持久化与恢复

**Day 1-3: 快照机制**

**任务清单**:
- [ ] 定期快照生成（每100操作）
- [ ] 快照存储与加载

**关键代码**:
```cpp
// SnapshotManager.h
class SnapshotManager {
public:
    void createSnapshot(const std::string& docId, const DocumentState& state);
    DocumentState loadSnapshot(const std::string& docId);
    void cleanupOldSnapshots(const std::string& docId, int keepCount);
    
private:
    bool shouldCreateSnapshot(uint64_t currentVersion, uint64_t lastSnapshotVersion);
};

// Snapshot表
CREATE TABLE snapshots (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    doc_id TEXT NOT NULL,
    version INTEGER NOT NULL,
    content TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(doc_id) REFERENCES documents(doc_id)
);
```

**交付物**:
- 快照生成逻辑
- 快照存储方案
- 清理策略

**验收标准**:
- 快照能正确生成
- 加载速度快
- 磁盘占用合理

---

**Day 4-5: 数据恢复**

**任务清单**:
- [ ] 从快照+增量操作重建
- [ ] 启动时自动恢复

**关键代码**:
```cpp
// RecoveryManager.h
class RecoveryManager {
public:
    DocumentState recoverDocument(const std::string& docId);
    
private:
    DocumentState rebuildFromSnapshot(const std::string& docId);
    std::vector<Operation> getOperationsAfterVersion(const std::string& docId, uint64_t version);
};

// RecoveryManager.cpp
DocumentState RecoveryManager::recoverDocument(const std::string& docId) {
    // 1. 加载最新快照
    auto state = rebuildFromSnapshot(docId);
    uint64_t snapshotVersion = state.getVersion();
    
    // 2. 获取快照后的所有操作
    auto ops = getOperationsAfterVersion(docId, snapshotVersion);
    
    // 3. 依次应用操作
    for (const auto& op : ops) {
        state.applyOperation(op);
    }
    
    return state;
}
```

**交付物**:
- 数据恢复逻辑
- 启动恢复机制

**验收标准**:
- 重启后数据完整
- 恢复速度快（< 1秒）

---

**Day 6-7: 数据清理**

**任务清单**:
- [ ] 操作日志清理策略
- [ ] 磁盘空间监控

**关键代码**:
```cpp
// DataCleanup.h
class DataCleanup {
public:
    void cleanupOldOperations(const std::string& docId, int daysToKeep);
    void cleanupOldSnapshots(const std::string& docId, int keepCount);
    uint64_t getDiskUsage();
    
private:
    void vacuumDatabase();
};
```

**交付物**:
- 数据清理工具
- 磁盘监控机制

**验收标准**:
- 旧数据能正确清理
- 磁盘空间可控

---

### 阶段3里程碑检查 (Week 6结束)

**Milestone 3 验收清单**:
- [ ] 离线编辑支持
- [ ] 数据持久化可靠
- [ ] 故障恢复正常
- [ ] 集成测试通过
- [ ] 快照机制工作正常
- [ ] 数据清理有效

---

### 阶段4: 完善与优化 (Week 7-8)

#### Week 7: API完善与前端集成

**Day 1-3: RESTful API完善**

**任务清单**:
- [ ] 实现所有文档管理API
- [ ] 实现用户认证API
- [ ] 实现房间管理API

**API列表**:
```
文档管理:
  POST   /api/documents              创建文档
  GET    /api/documents/:id          获取文档
  PUT    /api/documents/:id          更新文档
  DELETE /api/documents/:id          删除文档
  GET    /api/documents/:id/history  获取历史

用户认证:
  POST   /api/auth/register          注册
  POST   /api/auth/login             登录
  GET    /api/users/:id/profile      获取资料

房间管理:
  POST   /api/rooms                  创建房间
  GET    /api/rooms/:id              获取房间信息
  POST   /api/rooms/:id/join         加入房间
  POST   /api/rooms/:id/leave        离开房间
```

**交付物**:
- 完整的API实现
- API文档（Swagger格式）
- 错误码定义

**验收标准**:
- 所有API正常工作
- 错误处理完善
- 文档清晰

---

**Day 4-5: 简单前端Demo**

**任务清单**:
- [ ] HTML编辑器页面
- [ ] WebSocket连接逻辑
- [ ] 实时显示其他用户光标

**关键代码**:
```html
<!-- index.html -->
<!DOCTYPE html>
<html>
<head>
    <title>Collaborative Editor</title>
</head>
<body>
    <textarea id="editor" rows="20" cols="80"></textarea>
    <div id="cursors"></div>
    
    <script src="websocket-client.js"></script>
    <script src="editor.js"></script>
</body>
</html>
```

```javascript
// websocket-client.js
class CollaborativeClient {
    constructor(serverUrl) {
        this.ws = new WebSocket(serverUrl);
        this.setupEventHandlers();
    }
    
    setupEventHandlers() {
        this.ws.onmessage = (event) => {
            const message = JSON.parse(event.data);
            if (message.type === 'operation') {
                this.applyRemoteOperation(message.payload);
            }
        };
    }
    
    sendOperation(operation) {
        this.ws.send(JSON.stringify({
            type: 'operation',
            payload: operation
        }));
    }
}
```

**交付物**:
- 可运行的Web Demo
- WebSocket客户端库
- 用户界面

**验收标准**:
- 能在浏览器中运行
- 实时协作正常
- 界面友好

---

**Day 6-7: 端到端测试**

**任务清单**:
- [ ] 完整用户流程测试
- [ ] 浏览器兼容性测试

**测试流程**:
```
1. 用户A打开文档并开始编辑
2. 用户B加入同一房间
3. 两人同时编辑
4. 验证内容一致性
5. 用户A断开网络，继续编辑
6. 用户A恢复网络，验证同步
```

**交付物**:
- 端到端测试报告
- 浏览器兼容性报告

**验收标准**:
- 完整流程顺畅
- Chrome/Firefox兼容

---

#### Week 8: 性能优化与部署

**Day 1-2: 性能优化**

**任务清单**:
- [ ] 数据库索引优化
- [ ] 连接池调优
- [ ] 内存泄漏检查

**优化措施**:
```cpp
// 1. 对象池
class OperationPool {
    std::queue<std::shared_ptr<Operation>> pool;
public:
    std::shared_ptr<Operation> acquire() {
        if (pool.empty()) {
            return std::make_shared<Operation>();
        }
        auto op = pool.front();
        pool.pop();
        return op;
    }
    
    void release(std::shared_ptr<Operation> op) {
        pool.push(op);
    }
};

// 2. 批量写入
void batchInsertOperations(const std::vector<Operation>& ops) {
    db.beginTransaction();
    for (const auto& op : ops) {
        operationDAO.insert(op);
    }
    db.commitTransaction();
}
```

**交付物**:
- 性能优化报告
- 内存分析报告

**验收标准**:
- 内存无泄漏
- 性能提升明显

---

**Day 3-4: 监控与日志**

**任务清单**:
- [ ] 实现性能指标收集
- [ ] 添加健康检查endpoint
- [ ] 日志分级与轮转

**关键代码**:
```cpp
// Metrics.h
class Metrics {
public:
    void recordOperation();
    void recordLatency(uint64_t ms);
    void incrementActiveConnections();
    void decrementActiveConnections();
    
    std::string getMetricsJson();
    
private:
    std::atomic<uint64_t> totalOperations{0};
    std::atomic<uint64_t> activeConnections{0};
    std::vector<uint64_t> latencies;
    std::mutex metricsMutex;
};

// Health endpoint
CROW_ROUTE(app, "/health")
([]() {
    nlohmann::json health;
    health["status"] = "healthy";
    health["connections"] = metrics.getActiveConnections();
    health["uptime"] = getUptime();
    return crow::response(200, health.dump());
});
```

**交付物**:
- 监控系统
- 健康检查API
- 日志轮转配置

**验收标准**:
- 指标收集准确
- 健康检查正常
- 日志文件大小可控

---

**Day 5-6: 部署准备**

**任务清单**:
- [ ] Docker镜像构建
- [ ] systemd服务配置
- [ ] 生产环境配置文件

**Dockerfile**:
```dockerfile
FROM ubuntu:20.04

RUN apt-get update && apt-get install -y \
    libboost-all-dev \
    libsqlite3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN mkdir build && cd build && cmake .. && make

EXPOSE 8080
CMD ["./build/collab-engine", "--config", "config.json"]
```

**systemd服务**:
```ini
[Unit]
Description=Realtime Collaborative Engine
After=network.target

[Service]
Type=simple
User=collab
WorkingDirectory=/opt/collab-engine
ExecStart=/opt/collab-engine/collab-engine --config /opt/collab-engine/config.json
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

**交付物**:
- Docker镜像
- systemd服务文件
- 部署脚本

**验收标准**:
- Docker能正常运行
- systemd能管理服务
- 部署流程顺畅

---

**Day 7: 文档整理**

**任务清单**:
- [ ] 完善README
- [ ] 编写架构文档
- [ ] 编写部署指南

**文档列表**:
- README.md - 项目介绍和快速开始
- docs/architecture.md - 架构设计说明
- docs/api-reference.md - API参考文档
- docs/deployment-guide.md - 部署指南
- docs/development-guide.md - 开发指南

**交付物**:
- 完整的项目文档
- 架构图
- API文档

**验收标准**:
- 文档清晰完整
- 新人能快速上手

---

### 阶段4里程碑检查 (Week 8结束)

**Milestone 4 验收清单**:
- [ ] Web Demo可运行
- [ ] 性能指标达标
- [ ] 文档完整
- [ ] Docker部署成功
- [ ] 监控系统工作
- [ ] 所有测试通过

---

## 三、每日工作时间安排建议

```
上午 (9:00-12:00):
  - 9:00-9:30   回顾昨日进展，规划今日任务
  - 9:30-12:00  核心编码工作

下午 (13:30-17:30):
  - 13:30-15:30 继续编码
  - 15:30-16:30 编写测试用例
  - 16:30-17:30 调试和优化

晚上 (可选, 19:00-21:00):
  - 文档编写
  - 代码审查
  - 学习新技术
```

---

## 四、风险管理

### 4.1 技术风险

| 风险项 | 概率 | 影响 | 应对措施 |
|--------|------|------|----------|
| OT算法复杂性超预期 | 中 | 高 | 先实现简化版，逐步完善 |
| WebSocket并发瓶颈 | 低 | 中 | 使用Boost.Asio异步，压力测试 |
| 数据库写入瓶颈 | 中 | 中 | 批量写入+异步刷盘 |
| 内存泄漏 | 中 | 高 | 定期Valgrind检查 |

### 4.2 进度风险

| 风险项 | 应对措施 |
|--------|----------|
| 某个模块延期 | 预留20%缓冲时间 |
| 技术难点卡壳 | 寻求社区帮助，简化方案 |
| 需求变更 | 严格控制范围，MVP优先 |

---

## 五、质量保证

### 5.1 代码规范
- 遵循Google C++ Style Guide
- 使用clang-format格式化
- 每个函数必须有注释
- 变量命名见名知意

### 5.2 测试策略
- 单元测试覆盖率 > 80%
- 集成测试覆盖核心流程
- 压力测试验证性能指标
- 手动测试验证用户体验

### 5.3 代码审查
- 每个PR至少1人审查
- 重点审查：线程安全、内存管理、错误处理
- 使用静态分析工具（clang-tidy）

---

## 六、总结

本项目采用**渐进式开发**策略：
1. **Week 1-2**: 打好基础，确保基础设施稳固
2. **Week 3-4**: 攻克核心难点（OT算法）
3. **Week 5-6**: 完善高级功能
4. **Week 7-8**: 优化、测试、部署

**关键成功因素**:
- ✅ 严格的里程碑检查
- ✅ 充分的单元测试
- ✅ 持续的性能监控
- ✅ 及时的文档更新

**预期成果**:
- 一个可用的实时协作编辑引擎
- 完整的源代码和文档
- 可复用的OT算法实现
- 高性能网络编程经验

---

**文档版本**: v1.0  
**创建日期**: 2026-04-26  
**最后更新**: 2026-04-26

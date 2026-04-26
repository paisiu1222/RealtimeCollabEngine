# 实时协作编辑系统 - 功能需求文档

## 一、项目概述

### 1.1 项目名称
**Realtime-Collab-Engine** (实时协作编辑引擎)

### 1.2 项目定位
基于C++14的轻量级实时协作引擎，支持多人同时编辑共享文档，实现简化版OT(Operational Transformation)算法进行冲突解决。

### 1.3 核心价值
- **实用性**: 可应用于在线文档、协同笔记、代码协作等场景
- **学习价值**: 涵盖网络编程、并发控制、数据一致性、算法设计
- **可扩展性**: 模块化设计，便于后续添加新功能

### 1.4 技术选型
```
核心标准: C++14
网络层: Boost.Asio (异步IO + WebSocket)
数据存储: SQLite3 (本地持久化)
Web框架: Crow (HTTP API + WebSocket支持)
JSON处理: nlohmann/json
构建工具: CMake
```

---

## 二、功能模块详细需求

### 模块1: 用户会话管理 (Session Manager)

#### 1.1 功能描述
管理用户连接、认证和状态

#### 1.2 详细需求

**1.1 用户连接管理**
- WebSocket长连接建立与维护
- 心跳检测与断线重连机制
- 连接池管理（最大1000并发）

**1.2 用户认证**
- Token-based认证（JWT简化版）
- 会话超时自动清理（默认30分钟）
- 匿名用户支持（只读模式）

**1.3 房间管理**
- 创建/加入/离开协作房间
- 房间容量限制（默认50人/房间）
- 房间权限控制（公开/私有/密码保护）

#### 1.3 接口定义
```cpp
class SessionManager {
public:
    bool connectUser(const std::string& userId, WebSocketConnection& conn);
    void disconnectUser(const std::string& userId);
    bool joinRoom(const std::string& userId, const std::string& roomId);
    void leaveRoom(const std::string& userId);
    std::vector<std::string> getRoomUsers(const std::string& roomId);
};
```

---

### 模块2: 文档同步引擎 (Document Sync Engine)

#### 2.1 功能描述
核心同步逻辑，实现操作转换和冲突解决

#### 2.2 详细需求

**2.1 操作类型定义**
- `INSERT`: 插入文本
- `DELETE`: 删除文本
- `REPLACE`: 替换文本
- `FORMAT`: 格式变更（加粗、斜体等）

**2.2 OT算法实现（简化版）**
- 操作序列化与版本号管理
- 并发操作冲突检测
- 操作转换规则：
  - 插入-插入转换
  - 插入-删除转换
  - 删除-删除转换
- 最终一致性保证

**2.3 操作广播**
- 增量推送给房间内其他用户
- 操作确认机制（ACK）
- 失败重试与回滚

#### 2.3 数据结构
```cpp
struct Operation {
    enum Type { INSERT, DELETE, REPLACE, FORMAT };
    
    std::string opId;          // 操作唯一ID
    std::string userId;        // 操作用户
    uint64_t version;          // 文档版本号
    Type type;                 // 操作类型
    size_t position;           // 操作位置
    std::string content;       // 操作内容
    std::chrono::system_clock::time_point timestamp;
};

class DocumentState {
private:
    std::string content;       // 当前文档内容
    uint64_t currentVersion;   // 当前版本号
    std::deque<Operation> history; // 操作历史（保留最近1000条）
    
public:
    OperationResult applyOperation(const Operation& op);
    Operation transformOperation(const Operation& localOp, const Operation& remoteOp);
    std::string getContent() const;
    uint64_t getVersion() const;
};
```

---

### 模块3: 冲突解决与合并 (Conflict Resolver)

#### 3.1 功能描述
处理弱网环境下的并发冲突

#### 3.2 详细需求

**3.1 冲突检测**
- 基于版本号的并发检测
- 操作依赖关系分析
- 循环依赖检测

**3.2 自动合并策略**
- 时间戳优先策略（最后写入获胜）
- 位置无关操作并行执行
- 位置相关操作串行化处理

**3.3 离线编辑支持**
- 本地操作队列缓存
- 网络恢复后批量同步
- 冲突提示与手动解决界面（通过Web API）

#### 3.3 算法流程
```
用户A操作 → 检查版本号
    ├─ 版本一致 → 直接应用并广播
    └─ 版本落后 → 从服务器拉取缺失操作
                    ├─ 转换本地操作
                    ├─ 应用远程操作
                    └─ 重新应用本地操作
```

---

### 模块4: 持久化存储 (Persistence Layer)

#### 4.1 功能描述
文档快照和操作日志持久化

#### 4.2 详细需求

**4.1 数据库设计**
- `documents`表: 文档元信息
- `operations`表: 操作日志（WAL模式）
- `snapshots`表: 定期快照（每100次操作）
- `users`表: 用户信息

**4.2 性能优化**
- 批量写入（每10个操作或每秒）
- 异步刷盘
- 索引优化（version, room_id, user_id）

**4.3 数据恢复**
- 从最新快照+增量操作重建文档
- 操作日志清理策略（保留7天）

#### 4.3 SQL Schema
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

---

### 模块5: HTTP API服务 (RESTful API)

#### 5.1 功能描述
提供文档管理的HTTP接口

#### 5.2 详细需求

**5.1 文档管理API**
```
POST   /api/documents              创建文档
GET    /api/documents/:id          获取文档
PUT    /api/documents/:id          更新文档元信息
DELETE /api/documents/:id          删除文档
GET    /api/documents/:id/history  获取操作历史
```

**5.2 用户管理API**
```
POST   /api/auth/login             用户登录
POST   /api/auth/register          用户注册
GET    /api/users/:id/profile      获取用户信息
```

**5.3 房间管理API**
```
POST   /api/rooms                  创建房间
GET    /api/rooms/:id              获取房间信息
POST   /api/rooms/:id/join         加入房间
```

---

### 模块6: 监控与日志 (Monitoring & Logging)

#### 6.1 功能描述
系统运行状态监控

#### 6.2 详细需求

**6.1 性能指标**
- 活跃连接数
- 操作吞吐量（ops/sec）
- 平均同步延迟
- 冲突率统计

**6.2 日志记录**
- `INFO`: 用户连接、操作应用
- `WARNING`: 冲突检测、重试
- `ERROR`: 数据库错误、网络异常

**6.3 健康检查**
- `/health` endpoint
- 数据库连接状态
- 内存使用情况

---

## 三、非功能性需求

### 3.1 性能指标
```
- 单节点支持: 500-1000并发连接
- 操作延迟: P95 < 50ms (局域网)
- 吞吐量: 1000 ops/sec
- 内存占用: < 500MB (1000连接)
```

### 3.2 可靠性
```
- 数据持久化: 操作立即写入WAL
- 故障恢复: 重启后从快照恢复
- 断线重连: 自动重连并同步缺失操作
```

### 3.3 安全性
```
- Token认证: JWT简化版（HMAC-SHA256）
- 输入验证: SQL注入防护、XSS防护
- 速率限制: 每用户100 ops/min
```

---

## 四、项目目录结构

```
RealtimeCollabEngine/
├── CMakeLists.txt
├── README.md
├── config.json
├── docker-compose.yml
│
├── include/
│   ├── core/
│   │   ├── DocumentState.h
│   │   ├── Operation.h
│   │   ├── OTAlgorithm.h
│   │   └── ConflictResolver.h
│   │
│   ├── network/
│   │   ├── WebSocketServer.h
│   │   ├── MessageProtocol.h
│   │   └── SessionManager.h
│   │
│   ├── storage/
│   │   ├── Database.h
│   │   ├── DocumentDAO.h
│   │   ├── OperationDAO.h
│   │   └── SnapshotManager.h
│   │
│   ├── api/
│   │   ├── HttpServer.h
│   │   ├── AuthController.h
│   │   ├── DocumentController.h
│   │   └── RoomController.h
│   │
│   └── utils/
│       ├── Logger.h
│       ├── Config.h
│       └── Metrics.h
│
├── src/
│   ├── core/
│   ├── network/
│   ├── storage/
│   ├── api/
│   ├── utils/
│   └── main.cpp
│
├── tests/
│   ├── unit/
│   ├── integration/
│   └── fixtures/
│
├── web/
│   ├── index.html
│   ├── editor.js
│   └── websocket-client.js
│
├── scripts/
│   ├── init_db.sh
│   ├── run_tests.sh
│   └── deploy.sh
│
└── docs/
    ├── architecture.md
    ├── api-reference.md
    ├── deployment-guide.md
    └── development-guide.md
```

---

## 五、关键技术实现要点

### 5.1 OT算法简化策略

**核心思想**: 只处理文本级别的INSERT和DELETE操作

```cpp
// 简化版OT转换规则
Operation transform(const Operation& local, const Operation& remote) {
    if (local.type == INSERT && remote.type == INSERT) {
        if (local.position <= remote.position) {
            return local;
        } else {
            local.position += remote.content.length();
            return local;
        }
    }
    
    if (local.type == INSERT && remote.type == DELETE) {
        if (local.position <= remote.position) {
            return local;
        } else {
            local.position -= remote.content.length();
            return local;
        }
    }
    
    // ... 其他转换规则
}
```

### 5.2 WebSocket消息协议

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

### 5.3 性能优化建议

```cpp
// 1. 使用对象池减少内存分配
class OperationPool {
    std::queue<std::shared_ptr<Operation>> pool;
public:
    std::shared_ptr<Operation> acquire();
    void release(std::shared_ptr<Operation> op);
};

// 2. 批量写入数据库
void batchInsertOperations(const std::vector<Operation>& ops) {
    db.beginTransaction();
    for (const auto& op : ops) {
        operationDAO.insert(op);
    }
    db.commitTransaction();
}

// 3. 异步日志
class AsyncLogger {
    std::queue<std::string> logQueue;
    std::thread writerThread;
public:
    void log(const std::string& message);
};
```

---

## 六、可扩展性设计

### 6.1 插件架构

```cpp
// 操作处理器插件接口
class OperationHandler {
public:
    virtual ~OperationHandler() = default;
    virtual bool canHandle(const Operation& op) = 0;
    virtual OperationResult handle(Operation& op) = 0;
};

// 示例：富文本格式插件
class FormatHandler : public OperationHandler {
public:
    bool canHandle(const Operation& op) override {
        return op.type == Operation::FORMAT;
    }
    
    OperationResult handle(Operation& op) override {
        // 处理加粗、斜体等格式
    }
};
```

### 6.2 水平扩展方案

```
未来升级路径:
1. 单节点 → 多节点（Redis Pub/Sub广播）
2. 内存存储 → Redis缓存层
3. SQLite → PostgreSQL集群
4. 添加负载均衡器（Nginx）
```

---

## 七、风险评估与应对

| 风险项 | 概率 | 影响 | 应对措施 |
|--------|------|------|----------|
| OT算法复杂性超预期 | 中 | 高 | 先实现简化版，逐步完善 |
| WebSocket并发性能瓶颈 | 低 | 中 | 使用Boost.Asio异步模型，压力测试 |
| 数据库写入成为瓶颈 | 中 | 中 | 批量写入+异步刷盘 |
| 冲突解决逻辑bug | 高 | 高 | 充分单元测试+边界条件覆盖 |

---

## 八、验收标准

### 8.1 功能验收
- [ ] 支持至少2个用户同时编辑同一文档
- [ ] OT算法能正确处理并发冲突
- [ ] 离线编辑后能正确同步
- [ ] 数据持久化可靠，重启后可恢复

### 8.2 性能验收
- [ ] 支持100并发连接稳定运行
- [ ] P95延迟 < 100ms
- [ ] 吞吐量 > 500 ops/sec

### 8.3 代码质量
- [ ] 单元测试覆盖率 > 80%
- [ ] 无内存泄漏
- [ ] 代码规范符合C++14标准

---

**文档版本**: v1.0  
**创建日期**: 2026-04-26  
**最后更新**: 2026-04-26

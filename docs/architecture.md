# RealtimeCollabEngine 架构设计文档

## 📋 目录

1. [系统概述](#系统概述)
2. [架构设计](#架构设计)
3. [核心模块](#核心模块)
4. [数据流](#数据流)
5. [技术栈](#技术栈)
6. [性能指标](#性能指标)

---

## 系统概述

RealtimeCollabEngine 是一个基于 C++14 的轻量级实时协作编辑引擎，支持多用户同时编辑共享文档。

### 核心价值

- **高性能**: 基于 C++14 和异步 IO，支持高并发连接
- **低延迟**: P95 延迟 < 50ms（局域网环境）
- **数据一致性**: 通过 OT 算法保证多用户编辑的一致性
- **可扩展**: 模块化设计，易于扩展新功能

---

## 架构设计

### 整体架构图

```
┌─────────────────────────────────────────────────┐
│                  Client Layer                     │
│  (Web Browser / Mobile App / Desktop Client)     │
└──────────────┬──────────────────┬────────────────┘
               │ WebSocket        │ HTTP REST API
               ▼                  ▼
┌─────────────────────────────────────────────────┐
│              Network Layer                        │
│  ┌──────────────────┐  ┌──────────────────────┐ │
│  │ WebSocketServer  │  │   HttpServer (Crow)  │ │
│  └────────┬─────────┘  └──────────┬───────────┘ │
└───────────┼───────────────────────┼─────────────┘
            │                       │
            ▼                       ▼
┌─────────────────────────────────────────────────┐
│              Core Engine Layer                    │
│  ┌──────────┐ ┌──────────┐ ┌────────────────┐  │
│  │   Room   │ │SessionMgr│ │ OTAlgorithm    │  │
│  └──────────┘ └──────────┘ └────────────────┘  │
│  ┌──────────┐ ┌──────────┐ ┌────────────────┐  │
│  │DocState  │ │ Conflict │ │ Snapshot/Reco  │  │
│  └──────────┘ └──────────┘ └────────────────┘  │
└───────────┬───────────────────┬─────────────────┘
            │                   │
            ▼                   ▼
┌─────────────────────────────────────────────────┐
│              Storage Layer                        │
│  ┌──────────────┐  ┌──────────────────────────┐ │
│  │ DocumentDAO  │  │   OperationDAO           │ │
│  └──────┬───────┘  └──────────┬───────────────┘ │
│         └──────────┬──────────┘                  │
│                    ▼                              │
│         ┌──────────────────┐                     │
│         │  Database (SQLite)│                    │
│         └──────────────────┘                     │
└─────────────────────────────────────────────────┘
```

### 分层架构

#### 1. 接入层 (Network Layer)
- **WebSocketServer**: 处理实时双向通信
- **HttpServer**: 提供 RESTful API
- **MessageProtocol**: 消息序列化/反序列化
- **SessionManager**: 管理用户会话

#### 2. 核心逻辑层 (Core Engine Layer)
- **Room**: 房间管理，隔离不同协作文档
- **DocumentState**: 维护文档当前状态
- **OTAlgorithm**: 操作转换算法，解决冲突
- **ConflictResolver**: 冲突检测与解决
- **SnapshotManager**: 定期快照生成
- **RecoveryManager**: 从快照恢复文档

#### 3. 数据存储层 (Storage Layer)
- **Database**: SQLite3 连接池管理
- **DocumentDAO**: 文档数据访问对象
- **OperationDAO**: 操作日志数据访问对象

#### 4. 基础工具层 (Utils Layer)
- **Logger**: 线程安全日志系统
- **Config**: JSON 配置管理
- **Metrics**: 性能指标收集
- **LogRotator**: 日志轮转管理

---

## 核心模块

### 1. OT 算法模块

**职责**: 解决多用户并发编辑冲突

**关键类**:
- `Operation`: 定义编辑操作（插入、删除、替换）
- `DocumentState`: 维护文档状态和操作历史
- `OTAlgorithm`: 实现操作转换逻辑

**工作流程**:
```
用户A操作 → OT转换 → 应用并广播
用户B操作 → OT转换 → 应用并广播
```

### 2. 房间管理模块

**职责**: 隔离不同文档的协作会话

**关键类**:
- `Room`: 房间实体，包含用户列表和文档引用
- `SessionManager`: 管理所有活跃会话

**特性**:
- 用户加入/离开管理
- 消息广播（排除发送者）
- ACK 确认机制

### 3. 持久化模块

**职责**: 数据持久化和恢复

**关键类**:
- `SnapshotManager`: 定期创建文档快照
- `RecoveryManager`: 从快照+增量操作恢复文档

**策略**:
- 每 100 个操作创建一个快照
- 保留最近 5 个快照
- 恢复时从最近快照开始，应用后续操作

### 4. 监控模块

**职责**: 性能指标收集和日志管理

**关键类**:
- `Metrics`: 收集操作数、延迟、连接数等指标
- `LogRotator`: 自动轮转日志文件

**指标**:
- 总操作数
- 活跃连接数
- 延迟统计（P50, P95, P99）
- 错误计数

---

## 数据流

### 实时编辑流程

```
1. 客户端编辑文档
       ↓
2. 生成 Operation 对象
       ↓
3. 通过 WebSocket 发送到服务器
       ↓
4. WebSocketServer 接收消息
       ↓
5. MessageProtocol 解析消息
       ↓
6. SessionManager 路由到对应 Room
       ↓
7. OTAlgorithm 转换操作
       ↓
8. DocumentState 应用操作
       ↓
9. OperationDAO 持久化操作
       ↓
10. Room 广播给其他用户
       ↓
11. 其他客户端接收并应用操作
```

### 文档恢复流程

```
1. 服务重启或客户端重连
       ↓
2. RecoveryManager.recoverDocument(docId)
       ↓
3. SnapshotManager 加载最新快照
       ↓
4. 从快照重建 DocumentState
       ↓
5. OperationDAO 获取快照后的操作
       ↓
6. 依次应用每个操作
       ↓
7. 返回完整的 DocumentState
```

---

## 技术栈

### 核心技术

| 技术 | 版本 | 用途 |
|------|------|------|
| C++ | 14 | 核心语言 |
| CMake | 3.14+ | 构建系统 |
| Boost.Asio | 系统版本 | 异步网络 IO |
| Crow | 1.2.0 | HTTP/WebSocket 框架 |
| SQLite3 | 系统版本 | 本地数据库 |
| nlohmann/json | 系统版本 | JSON 处理 |
| OpenSSL | 系统版本 | SSL/TLS 加密 |

### 开发工具

- **编译器**: GCC / Clang / MSVC
- **调试器**: GDB / LLDB
- **性能分析**: Valgrind, perf
- **代码格式**: clang-format

---

## 性能指标

### 目标指标

| 指标 | 目标值 | 说明 |
|------|--------|------|
| 并发连接 | 500-1000 | 同时在线用户数 |
| 操作延迟 (P95) | < 50ms | 局域网环境 |
| 吞吐量 | 1000 ops/sec | 每秒处理操作数 |
| 内存占用 | < 500MB | 1000 连接时 |
| 恢复时间 | < 1s | 从快照恢复文档 |

### 优化策略

1. **异步 IO**: 使用 Boost.Asio 非阻塞网络操作
2. **连接池**: 复用数据库连接，减少开销
3. **批量写入**: 事务方式批量插入操作
4. **快照机制**: 避免 replay 所有历史操作
5. **内存管理**: 使用对象池减少分配开销

---

## 部署架构

### 单机部署

```
┌─────────────────────────┐
│   RealtimeCollabEngine  │
│  ┌───────────────────┐  │
│  │  WebSocket Server │  │
│  │  HTTP Server      │  │
│  │  Core Engine      │  │
│  │  SQLite Database  │  │
│  └───────────────────┘  │
└─────────────────────────┘
```

### Docker 部署

```bash
# 构建镜像
docker build -t collab-engine .

# 运行容器
docker run -d \
  -p 8080:8080 \
  -v ./logs:/app/logs \
  -v ./data:/app/data \
  --name collab-engine \
  collab-engine
```

### systemd 部署

```bash
# 安装服务
sudo ./deploy/deploy.sh install

# 启动服务
sudo systemctl start collab-engine

# 查看状态
sudo systemctl status collab-engine
```

---

## 安全考虑

1. **SSL/TLS**: 支持 HTTPS/WSS 加密通信
2. **认证**: JWT Token 验证（待实现）
3. **授权**: 基于角色的访问控制（待实现）
4. **输入验证**: 所有 API 输入参数验证
5. **资源限制**: 连接数、内存、CPU 限制

---

## 未来改进方向

1. **分布式架构**: 支持多节点集群
2. **Redis 缓存**: 热点数据缓存
3. **消息队列**: 异步操作处理
4. **监控告警**: Prometheus + Grafana
5. **CI/CD**: 自动化测试和部署

---

**文档版本**: v1.0  
**最后更新**: 2026-04-28  
**作者**: RealtimeCollabEngine Team

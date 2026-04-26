# RealtimeCollabEngine - 实时协作编辑引擎

基于C++14的轻量级实时协作引擎，支持多人同时编辑共享文档，实现简化版OT(Operational Transformation)算法进行冲突解决。

## 📋 项目特性

- ✅ **实时协作**: 支持多用户同时编辑同一文档
- ✅ **OT算法**: 实现操作转换算法解决并发冲突
- ✅ **WebSocket**: 基于Boost.Asio的异步WebSocket通信
- ✅ **数据持久化**: SQLite3本地存储
- ✅ **RESTful API**: 基于Crow框架的HTTP API
- ✅ **跨平台**: 支持Linux、macOS、Windows

## 🛠️ 技术栈

- **核心标准**: C++14
- **网络层**: Boost.Asio (异步IO + WebSocket)
- **数据存储**: SQLite3 (本地持久化)
- **Web框架**: Crow (HTTP API + WebSocket支持)
- **JSON处理**: nlohmann/json
- **构建工具**: CMake

## 📦 依赖要求

### 系统依赖

```

```

## 🚀 快速开始

### 1. 克隆项目

```bash
git clone <repository-url>
cd RealtimeCollabEngine
```

### 2. 安装依赖

```bash
# Ubuntu/Debian
sudo apt-get install -y cmake build-essential libboost-all-dev libsqlite3-dev \
    nlohmann-json3-dev libasio-dev libssl-dev

# Crow框架（从源码安装）
git clone --depth 1 --branch v1.2.0 https://github.com/CrowCpp/Crow.git
cd Crow && mkdir build && cd build
cmake .. -DCROW_BUILD_EXAMPLES=OFF -DCROW_BUILD_TESTS=OFF -DCROW_AMALGAMATE=ON
sudo make install
```

### 3. 构建项目

```bash
# 标准构建
./build.sh Release OFF

# 带测试的构建
./build.sh Debug ON
```

### 4. 运行程序

```bash
# 直接运行
./run.sh

# 或手动运行
./build/bin/RealtimeCollabEngine
```

### 5. 运行测试

```bash
# 运行所有测试
./test.sh

# 或单独运行
cd build
./bin/test_logger
./bin/test_config
```

### 4. 配置文件

编辑 `config.json` 文件来配置服务器参数：

```json
{
    "server": {
        "host": "0.0.0.0",
        "port": 8080,
        "max_connections": 1000
    },
    "database": {
        "path": "collab_engine.db"
    }
}
```

## 📁 项目结构

```
RealtimeCollabEngine/
├── CMakeLists.txt          # CMake构建配置
├── config.json             # 配置文件
├── README.md               # 项目说明
│
├── include/                # 头文件目录
│   ├── core/              # 核心模块
│   │   ├── DocumentState.h
│   │   ├── Operation.h
│   │   ├── OTAlgorithm.h
│   │   └── ConflictResolver.h
│   ├── network/           # 网络模块
│   │   ├── WebSocketServer.h
│   │   ├── MessageProtocol.h
│   │   └── SessionManager.h
│   ├── storage/           # 存储模块
│   │   ├── Database.h
│   │   ├── DocumentDAO.h
│   │   └── OperationDAO.h
│   ├── api/               # API模块
│   │   ├── HttpServer.h
│   │   └── Controllers.h
│   └── utils/             # 工具类
│       ├── Logger.h
│       └── Config.h
│
├── src/                   # 源文件目录
│   ├── core/
│   ├── network/
│   ├── storage/
│   ├── api/
│   ├── utils/
│   └── main.cpp
│
├── tests/                 # 测试文件
│   ├── unit/
│   └── integration/
│
└── docs/                  # 文档
    ├── architecture.md
    └── api-reference.md
```

## 🔧 开发指南

### 编译选项

```bash
# Debug模式（包含调试信息）
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Release模式（优化）
cmake .. -DCMAKE_BUILD_TYPE=Release

# 启用单元测试
cmake .. -DBUILD_TESTS=ON
```

### 运行测试

```bash
# 编译测试
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)

# 运行测试
ctest
```

## 📊 性能指标

- **并发连接**: 500-1000
- **操作延迟**: P95 < 50ms (局域网)
- **吞吐量**: 1000 ops/sec
- **内存占用**: < 500MB (1000连接)

## 📝 开发周期

预计开发周期：**8周**

- **Week 1-2**: 基础设施搭建
- **Week 3-4**: 核心同步引擎
- **Week 5-6**: 高级功能
- **Week 7-8**: 测试与优化

详细开发计划请查看 [开发周期文档](RealtimeCollabEngine_开发周期文档.md)

## 🤝 贡献

欢迎提交Issue和Pull Request！

## 📄 许可证

本项目采用 MIT 许可证

## 👥 作者

- Your Name

---

**文档版本**: v1.0  
**最后更新**: 2026-04-26

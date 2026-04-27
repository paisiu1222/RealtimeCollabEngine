# RealtimeCollabEngine 部署指南

## 📋 目录

1. [系统要求](#系统要求)
2. [快速开始](#快速开始)
3. [Docker 部署](#docker-部署)
4. [systemd 部署](#systemd-部署)
5. [手动部署](#手动部署)
6. [配置说明](#配置说明)
7. [监控与维护](#监控与维护)
8. [故障排查](#故障排查)

---

## 系统要求

### 硬件要求

| 组件 | 最低配置 | 推荐配置 |
|------|----------|----------|
| CPU | 2 核心 | 4+ 核心 |
| 内存 | 2 GB | 4+ GB |
| 磁盘 | 10 GB | 20+ GB SSD |
| 网络 | 100 Mbps | 1 Gbps |

### 软件要求

- **操作系统**: Ubuntu 20.04+ / CentOS 7+ / Debian 10+
- **编译器**: GCC 7+ 或 Clang 5+
- **CMake**: 3.14+
- **依赖库**: Boost, SQLite3, OpenSSL, Crow

---

## 快速开始

### 方法一：Docker Compose（推荐）

```bash
# 1. 克隆项目
git clone https://github.com/paisiu1222/RealtimeCollabEngine.git
cd RealtimeCollabEngine

# 2. 启动服务
docker-compose up -d

# 3. 查看日志
docker-compose logs -f

# 4. 验证服务
curl http://localhost:8080/health
```

### 方法二：systemd 服务

```bash
# 1. 克隆项目
git clone https://github.com/paisiu1222/RealtimeCollabEngine.git
cd RealtimeCollabEngine

# 2. 安装服务（需要 root 权限）
sudo ./deploy/deploy.sh install

# 3. 启动服务
sudo systemctl start collab-engine

# 4. 设置开机自启
sudo systemctl enable collab-engine

# 5. 查看状态
sudo systemctl status collab-engine
```

---

## Docker 部署

### 构建镜像

```bash
# 构建 Docker 镜像
docker build -t collab-engine:latest .

# 查看镜像
docker images | grep collab-engine
```

### 运行容器

**基础运行:**
```bash
docker run -d \
  --name collab-engine \
  -p 8080:8080 \
  -v $(pwd)/logs:/app/logs \
  -v $(pwd)/data:/app/data \
  collab-engine:latest
```

**使用 Docker Compose:**
```yaml
# docker-compose.yml
version: '3.8'

services:
  collab-engine:
    image: collab-engine:latest
    container_name: realtime-collab-engine
    ports:
      - "8080:8080"
    volumes:
      - ./logs:/app/logs
      - ./data:/app/data
      - ./config.json:/app/config.json:ro
    restart: unless-stopped
    environment:
      - TZ=Asia/Shanghai
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:8080/health"]
      interval: 30s
      timeout: 10s
      retries: 3
```

```bash
# 启动服务
docker-compose up -d

# 查看日志
docker-compose logs -f collab-engine

# 停止服务
docker-compose down

# 重启服务
docker-compose restart
```

### 数据持久化

```bash
# 创建数据卷
docker volume create collab-data
docker volume create collab-logs

# 使用数据卷
docker run -d \
  -v collab-data:/app/data \
  -v collab-logs:/app/logs \
  collab-engine:latest
```

---

## systemd 部署

### 安装步骤

```bash
# 1. 准备环境
sudo apt-get update
sudo apt-get install -y cmake build-essential libboost-all-dev \
    libsqlite3-dev nlohmann-json3-dev libasio-dev libssl-dev \
    pkg-config uuid-dev zlib1g-dev

# 2. 安装 Crow
git clone --depth 1 --branch v1.2.0 https://github.com/CrowCpp/Crow.git
cd Crow && mkdir build && cd build
cmake .. -DCROW_BUILD_EXAMPLES=OFF -DCROW_BUILD_TESTS=OFF -DCROW_AMALGAMATE=ON
sudo make install
cd ../.. && rm -rf Crow

# 3. 部署应用
git clone https://github.com/paisiu1222/RealtimeCollabEngine.git
cd RealtimeCollabEngine
sudo ./deploy/deploy.sh install

# 4. 配置生产环境
sudo cp config.production.json /opt/collab-engine/config.json
sudo chown collab:collab /opt/collab-engine/config.json
```

### 服务管理

```bash
# 启动服务
sudo systemctl start collab-engine

# 停止服务
sudo systemctl stop collab-engine

# 重启服务
sudo systemctl restart collab-engine

# 查看状态
sudo systemctl status collab-engine

# 查看日志
sudo journalctl -u collab-engine -f

# 设置开机自启
sudo systemctl enable collab-engine

# 禁用开机自启
sudo systemctl disable collab-engine
```

### 资源限制

编辑 `/etc/systemd/system/collab-engine.service`:

```ini
[Service]
# 内存限制
MemoryMax=1G

# CPU 限制（80%）
CPUQuota=80%

# 文件描述符限制
LimitNOFILE=65536
```

```bash
# 重新加载配置
sudo systemctl daemon-reload
sudo systemctl restart collab-engine
```

---

## 手动部署

### 编译安装

```bash
# 1. 安装依赖
sudo apt-get install -y cmake build-essential libboost-all-dev \
    libsqlite3-dev nlohmann-json3-dev libasio-dev libssl-dev \
    pkg-config uuid-dev zlib1g-dev

# 2. 安装 Crow
git clone --depth 1 --branch v1.2.0 https://github.com/CrowCpp/Crow.git
cd Crow && mkdir build && cd build
cmake .. -DCROW_BUILD_EXAMPLES=OFF -DCROW_BUILD_TESTS=OFF -DCROW_AMALGAMATE=ON
sudo make install

# 3. 克隆并编译项目
git clone https://github.com/paisiu1222/RealtimeCollabEngine.git
cd RealtimeCollabEngine
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# 4. 创建目录结构
cd ..
mkdir -p logs data

# 5. 运行服务
./build/bin/RealtimeCollabEngine
```

### 后台运行

```bash
# 使用 nohup
nohup ./build/bin/RealtimeCollabEngine > logs/server.log 2>&1 &

# 使用 screen
screen -S collab-engine
./build/bin/RealtimeCollabEngine
# Ctrl+A, D 退出 screen

# 恢复 screen
screen -r collab-engine
```

---

## 配置说明

### 配置文件位置

- **开发环境**: `config.json`
- **生产环境**: `config.production.json`

### 主要配置项

```json
{
    "server": {
        "host": "0.0.0.0",              // 监听地址
        "port": 8080,                   // HTTP 端口
        "websocket_host": "0.0.0.0",    // WebSocket 监听地址
        "websocket_port": 8080,         // WebSocket 端口
        "max_connections": 1000,        // 最大连接数
        "ssl_enabled": false,           // 是否启用 SSL
        "ssl_cert_path": "",            // SSL 证书路径
        "ssl_key_path": ""              // SSL 私钥路径
    },
    "database": {
        "path": "data/collab_engine.db", // 数据库文件路径
        "pool_size": 5,                  // 连接池大小
        "journal_mode": "WAL",           // WAL 模式提升并发性能
        "synchronous": "NORMAL"          // 同步级别
    },
    "logging": {
        "level": "INFO",                // 日志级别: DEBUG, INFO, WARNING, ERROR
        "log_dir": "logs",              // 日志目录
        "max_file_size_mb": 10,         // 单文件最大大小
        "max_backup_count": 5           // 保留备份数量
    },
    "snapshot": {
        "enabled": true,                // 启用快照
        "interval_operations": 100,     // 快照间隔（操作数）
        "max_snapshots_per_doc": 5      // 每文档最大快照数
    }
}
```

### SSL/TLS 配置

```json
{
    "server": {
        "ssl_enabled": true,
        "ssl_cert_path": "/etc/ssl/certs/collab.crt",
        "ssl_key_path": "/etc/ssl/private/collab.key"
    }
}
```

生成自签名证书：
```bash
openssl req -x509 -newkey rsa:4096 -keyout collab.key \
    -out collab.crt -days 365 -nodes
```

---

## 监控与维护

### 健康检查

```bash
# API 健康检查
curl http://localhost:8080/health

# 响应示例
{
  "status": "healthy",
  "uptime": 3600,
  "connections": 50,
  "version": "1.0.0"
}
```

### 性能指标

```bash
# 获取详细指标
curl http://localhost:8080/metrics

# 响应示例
{
  "uptime_seconds": 3600,
  "total_operations": 10000,
  "active_connections": 50,
  "total_errors": 5,
  "latency": {
    "avg_ms": 12,
    "p50_ms": 10,
    "p95_ms": 25,
    "p99_ms": 45,
    "min_ms": 2,
    "max_ms": 120,
    "count": 1000
  }
}
```

### 日志管理

```bash
# 查看实时日志
tail -f logs/server.log

# 查看错误日志
grep ERROR logs/server.log

# 清理旧日志
find logs/ -name "*.log.*" -mtime +30 -delete
```

### 数据库维护

```bash
# 备份数据库
cp data/collab_engine.db data/collab_engine.db.backup.$(date +%Y%m%d)

# 压缩数据库
sqlite3 data/collab_engine.db "VACUUM;"

# 检查数据库完整性
sqlite3 data/collab_engine.db "PRAGMA integrity_check;"
```

---

## 故障排查

### 常见问题

#### 1. 服务无法启动

```bash
# 检查端口占用
sudo lsof -i :8080

# 查看详细错误日志
sudo journalctl -u collab-engine -n 100

# 检查配置文件
cat /opt/collab-engine/config.json | python3 -m json.tool
```

#### 2. 连接超时

```bash
# 检查防火墙
sudo ufw status
sudo ufw allow 8080/tcp

# 检查网络连接
ping localhost
telnet localhost 8080
```

#### 3. 数据库锁定

```bash
# 检查数据库文件权限
ls -lh data/collab_engine.db*

# 修复权限
sudo chown collab:collab data/*.db*
sudo chmod 644 data/*.db*

# 删除锁文件
rm -f data/collab_engine.db-wal data/collab_engine.db-shm
```

#### 4. 内存不足

```bash
# 查看内存使用
free -h
top -p $(pgrep collab-engine)

# 调整 systemd 内存限制
sudo systemctl edit collab-engine
# 添加: MemoryMax=2G
```

### 性能调优

#### 数据库优化

```sql
-- 启用 WAL 模式
PRAGMA journal_mode=WAL;

-- 设置同步级别
PRAGMA synchronous=NORMAL;

-- 增加缓存大小
PRAGMA cache_size=-64000;  -- 64MB
```

#### 连接池调优

```json
{
    "database": {
        "pool_size": 10,  // 根据并发连接数调整
        "journal_mode": "WAL",
        "synchronous": "NORMAL"
    }
}
```

---

## 升级指南

### 从旧版本升级

```bash
# 1. 备份数据
cp -r data data.backup
cp -r logs logs.backup

# 2. 停止服务
sudo systemctl stop collab-engine

# 3. 拉取最新代码
cd /opt/collab-engine
git pull origin main

# 4. 重新编译
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# 5. 启动服务
sudo systemctl start collab-engine

# 6. 验证服务
curl http://localhost:8080/health
```

### 回滚

```bash
# 如果升级失败，回滚到备份
sudo systemctl stop collab-engine
rm -rf data logs
mv data.backup data
mv logs.backup logs
sudo systemctl start collab-engine
```

---

## 安全建议

1. **启用 SSL/TLS**: 生产环境必须启用 HTTPS/WSS
2. **防火墙配置**: 仅开放必要端口
3. **定期更新**: 及时更新系统和依赖库
4. **日志审计**: 定期检查日志发现异常
5. **备份策略**: 定期备份数据库和配置文件
6. **访问控制**: 实施用户认证和授权（待实现）

---

**文档版本**: v1.0  
**最后更新**: 2026-04-28  
**维护者**: RealtimeCollabEngine Team

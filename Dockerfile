# 使用Ubuntu 20.04作为基础镜像
FROM ubuntu:20.04

# 设置环境变量避免交互式提示
ENV DEBIAN_FRONTEND=noninteractive

# 安装系统依赖
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    libsqlite3-dev \
    nlohmann-json3-dev \
    libasio-dev \
    libssl-dev \
    pkg-config \
    uuid-dev \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# 安装Crow框架
RUN git clone --depth 1 --branch v1.2.0 https://github.com/CrowCpp/Crow.git /tmp/crow \
    && cd /tmp/crow \
    && mkdir build && cd build \
    && cmake .. -DCROW_BUILD_EXAMPLES=OFF -DCROW_BUILD_TESTS=OFF -DCROW_AMALGAMATE=ON \
    && make install \
    && rm -rf /tmp/crow

# 设置工作目录
WORKDIR /app

# 复制项目文件
COPY . .

# 构建项目
RUN mkdir -p build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release \
    && make -j$(nproc)

# 暴露端口
EXPOSE 8080

# 创建日志和数据目录
RUN mkdir -p /app/logs /app/data

# 设置卷
VOLUME ["/app/logs", "/app/data"]

# 启动命令
CMD ["./build/bin/RealtimeCollabEngine"]

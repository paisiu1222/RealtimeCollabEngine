#!/bin/bash

# RealtimeCollabEngine 快速启动脚本

set -e

echo "=========================================="
echo "  Starting RealtimeCollabEngine"
echo "=========================================="
echo ""

# 检查可执行文件是否存在
if [ ! -f "./build/bin/RealtimeCollabEngine" ]; then
    echo "Error: Executable not found. Building first..."
    ./build.sh Debug OFF
fi

# 检查配置文件
if [ ! -f "./config.json" ]; then
    echo "Warning: config.json not found. Using default configuration."
fi

# 创建日志目录
mkdir -p logs

# 运行程序
echo "Starting server..."
echo ""
cd build && ./bin/RealtimeCollabEngine

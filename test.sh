#!/bin/bash

# RealtimeCollabEngine 测试运行脚本

set -e

echo "=========================================="
echo "  Running All Tests"
echo "=========================================="
echo ""

# 检查build目录
if [ ! -d "build" ]; then
    echo "Error: Build directory not found. Please run ./build.sh first."
    exit 1
fi

cd build

# 运行所有测试
echo "Running ctest..."
echo ""
ctest --output-on-failure

echo ""
echo "=========================================="
echo "  All Tests Completed"
echo "=========================================="

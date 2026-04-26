#!/bin/bash

# RealtimeCollabEngine 构建脚本

set -e

echo "=========================================="
echo "  RealtimeCollabEngine Build Script"
echo "=========================================="
echo ""

# 检测操作系统
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
else
    echo "Unsupported OS: $OSTYPE"
    exit 1
fi

echo "Detected OS: $OS"
echo ""

# 解析命令行参数
BUILD_TYPE=${1:-Release}
BUILD_TESTS=${2:-OFF}

echo "Build Type: $BUILD_TYPE"
echo "Build Tests: $BUILD_TESTS"
echo ""

# 创建build目录
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir -p build
fi

# 进入build目录并运行CMake
cd build
echo "Running CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DBUILD_TESTS=$BUILD_TESTS

# 编译
echo ""
echo "Building..."
if [[ "$OS" == "macos" ]]; then
    MAKE_CMD="make -j$(sysctl -n hw.ncpu)"
else
    MAKE_CMD="make -j$(nproc)"
fi

$MAKE_CMD

echo ""
echo "=========================================="
echo "  Build Complete!"
echo "=========================================="
echo ""
echo "Executable: ./bin/RealtimeCollabEngine"
echo ""

# 如果启用了测试，运行测试
if [ "$BUILD_TESTS" = "ON" ]; then
    echo "Running tests..."
    ctest --output-on-failure
    echo ""
fi

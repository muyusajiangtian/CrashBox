#!/bin/bash
# CrashBox 一键构建并运行脚本
# 用法: ./run.sh [clean|build|run]

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
EXECUTABLE="$BUILD_DIR/CrashBox.exe"

echo "=============================="
echo "  CrashBox 构建运行工具"
echo "=============================="

if ! command -v cmake &> /dev/null; then
    echo "[错误] 未找到cmake"
    exit 1
fi

ACTION="${1:-all}"

clean() {
    echo "[清理] 删除构建目录..."
    rm -rf "$BUILD_DIR"
    echo "[清理] 完成"
}

build() {
    echo "[构建] 配置项目..."
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    cmake .. -G "MinGW Makefiles" \
        -DCMAKE_BUILD_TYPE=Release \
        -DSFML_DIR="E:/SFML/SFML-3.0.1-gcc15/lib/cmake/SFML"

    echo "[构建] 编译中..."
    cmake --build . --parallel $(nproc 2>/dev/null || echo 4)

    # 复制配置文件
    cp -r "$PROJECT_DIR/config" "$BUILD_DIR/" 2>/dev/null || true

    if [ -f "$EXECUTABLE" ]; then
        echo "[构建] 成功: $EXECUTABLE"
    else
        echo "[错误] 构建失败"
        exit 1
    fi
}

run() {
    if [ ! -f "$EXECUTABLE" ]; then
        echo "[提示] 可执行文件不存在，先执行构建"
        build
    fi
    echo "[运行] 启动 CrashBox..."
    cd "$BUILD_DIR"
    ./CrashBox.exe
}

case "$ACTION" in
    clean) clean ;;
    build) build ;;
    run)   run ;;
    all|"") build; run ;;
    *)
        echo "用法: $0 [clean|build|run|all]"
        exit 1
        ;;
esac

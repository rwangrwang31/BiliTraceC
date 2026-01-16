#!/bin/bash
# ============================================================
# BiliTraceC Linux/macOS 编译脚本
# ============================================================

echo ""
echo "╔══════════════════════════════════════════════════════════╗"
echo "║     BiliTraceC - Linux/macOS 编译脚本                    ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

# 检测操作系统
OS=$(uname -s)
echo "[系统] 检测到: $OS"

# 检查依赖
check_dependency() {
    if ! command -v $1 &> /dev/null; then
        echo "[错误] 未找到 $1"
        return 1
    fi
    return 0
}

# 检查编译器
if ! check_dependency gcc; then
    echo "请安装 GCC 编译器"
    if [ "$OS" = "Darwin" ]; then
        echo "  macOS: xcode-select --install"
    else
        echo "  Ubuntu: sudo apt-get install build-essential"
        echo "  CentOS: sudo yum groupinstall 'Development Tools'"
    fi
    exit 1
fi

# 检查curl
if ! pkg-config --exists libcurl 2>/dev/null; then
    echo "[警告] 未找到 libcurl 开发库"
    if [ "$OS" = "Darwin" ]; then
        echo "  macOS: brew install curl"
    else
        echo "  Ubuntu: sudo apt-get install libcurl4-openssl-dev"
        echo "  CentOS: sudo yum install libcurl-devel"
    fi
    
    # 尝试使用默认路径
    CURL_CFLAGS=""
    CURL_LIBS="-lcurl"
else
    CURL_CFLAGS=$(pkg-config --cflags libcurl)
    CURL_LIBS=$(pkg-config --libs libcurl)
fi

echo "[编译] 开始编译..."
echo ""

# 编译
gcc -O3 -Wall -Wextra -pthread $CURL_CFLAGS \
    -o bilitrace \
    main.c cracker.c network.c \
    $CURL_LIBS -pthread

if [ $? -eq 0 ]; then
    echo ""
    echo "╔══════════════════════════════════════════════════════════╗"
    echo "║  [成功] 编译完成: bilitrace                              ║"
    echo "╚══════════════════════════════════════════════════════════╝"
    echo ""
    echo "运行示例:"
    echo "  ./bilitrace -hash bc28c067"
    echo "  ./bilitrace -cid 123456789"
    echo ""
    
    # 提示安装
    echo "如需安装到系统:"
    echo "  sudo make install"
    echo ""
else
    echo ""
    echo "[错误] 编译失败!"
    exit 1
fi

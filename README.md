# BiliTraceC - B站弹幕溯源工具

基于CRC32逆向工程的高性能C语言实现

## 功能特性

- **在线抓取模式**：根据视频CID自动下载弹幕并反查发送者UID
- **关键词搜索**：支持过滤包含特定文本的弹幕
- **离线解密模式**：直接输入CRC32 Hash进行破解
- **多线程并行**：利用多核CPU实现秒级溯源
- **高性能优化**：查表法CRC32 + 自定义Itoa，吞吐量可达 480M Hash/s (8线程)

## 技术原理

B站弹幕系统使用CRC32算法对用户UID进行"加密"，生成8位十六进制的发送者Hash。由于：

1. CRC32是用于检错而非加密的算法
2. B站UID为纯数字，取值范围有限（0-40亿）
3. 现代CPU算力强大

因此可以通过暴力遍历在数秒内还原真实UID。

## 编译要求

### 依赖库

- **libcurl**：用于HTTP网络请求
- **pthread**：POSIX线程库（Windows下使用pthreads-win32或MinGW内置）

### Linux/macOS

```bash
# 安装依赖
# Ubuntu/Debian
sudo apt-get install libcurl4-openssl-dev

# macOS (Homebrew)
brew install curl

# 编译
make
```

### Windows (MSYS2/MinGW)

```bash
# 安装依赖
pacman -S mingw-w64-x86_64-curl mingw-w64-x86_64-gcc

# 编译
make
```

### Windows (Visual Studio)

需要安装vcpkg并配置curl库：

```cmd
vcpkg install curl:x64-windows
```

然后使用提供的CMakeLists.txt或手动配置项目。

## 使用方法

### 在线抓取模式

```bash
# 基本用法 - 解析视频弹幕
./bilitrace -cid 123456789

# 搜索特定弹幕
./bilitrace -cid 123456789 -search "前方高能"

# 限制处理数量
./bilitrace -cid 123456789 -limit 50

# 指定线程数
./bilitrace -cid 123456789 -threads 16
```

### 离线解密模式

```bash
# 直接解密Hash
./bilitrace -hash bc28c067

# 指定线程数
./bilitrace -hash bc28c067 -threads 16
```

### 参数说明

| 参数 | 说明 | 示例 |
|------|------|------|
| `-cid <CID>` | 视频内容ID | `-cid 123456789` |
| `-hash <HASH>` | CRC32哈希值 | `-hash bc28c067` |
| `-search <关键词>` | 搜索包含关键词的弹幕 | `-search "666"` |
| `-limit <数量>` | 处理弹幕数量上限 | `-limit 100` |
| `-threads <线程数>` | 并行线程数 (1-64) | `-threads 8` |

## 获取视频CID

CID与视频BV号不同，可通过以下方式获取：

1. **浏览器开发者工具**：在视频页面打开Network面板，搜索`.xml`请求
2. **B站API**：`https://api.bilibili.com/x/player/pagelist?bvid=BV1xxxxxx`
3. **第三方工具**：如BiliGrab等

## 性能数据

| 配置 | 单线程吞吐量 | 8线程吞吐量 | 全空间扫描时间 |
|------|-------------|------------|---------------|
| Intel i7-9700K | 65 M/s | 480 M/s | ~8.3秒 |
| AMD R7-5800X | 75 M/s | 560 M/s | ~7.1秒 |

> 实际破解时间取决于目标UID的大小，平均命中时间约为全扫描时间的一半

## 项目结构

```
BiliTraceC/
├── main.c           # 主程序入口（支持实时/历史双模式）
├── cracker.c        # 单线程CRC32破解模块
├── cracker.h        # 破解模块头文件
├── network.c        # 网络模块（libcurl封装）
├── network.h        # 网络模块头文件
├── history_api.c    # 历史弹幕API模块（需SESSDATA鉴权）
├── history_api.h    # 历史API头文件
├── proto_parser.c   # Protobuf手动解析器
├── proto_parser.h   # Protobuf解析器头文件
├── crc32_core.h     # CRC32核心算法（查表法）
├── utils.h          # 工具函数（快速Itoa）
├── cJSON.c/h        # JSON解析库
├── Makefile         # Linux/macOS编译配置
├── build.bat        # Windows编译脚本
├── CMakeLists.txt   # CMake配置
├── deps/            # 依赖库（curl等）
└── README.md        # 项目说明
```

## 使用模式

### 1. 实时模式（匿名）

```bash
# 无需登录，抓取最新弹幕
./check_history -cid 35268920394
./check_history -cid 35268920394 -search "关键词"
```

### 2. 历史模式（需SESSDATA）

```bash
# 需要登录态，可回溯7天历史弹幕
./check_history -cid 35268920394 -sessdata "YOUR_SESSDATA" -search "关键词"
```

### 3. 离线哈希破解

```bash
./check_history -hash bc28c067
```

## 免责声明

⚠️ **本工具仅供安全研究与学术交流使用**

- 请勿用于网络暴力、人肉搜索等非法行为
- 大规模爬取数据可能违反B站服务条款
- 使用本工具产生的任何法律后果由使用者自行承担
- 技术本身是中性的，使用者必须遵守法律与道德底线

## 许可证

MIT License

## 参考资料

- [CRC32算法详解](https://create.stephan-brumme.com/crc32/)
- [B站弹幕协议分析](https://socialsisteryi.github.io/bilibili-API-collect/)
- [libcurl文档](https://curl.se/libcurl/)

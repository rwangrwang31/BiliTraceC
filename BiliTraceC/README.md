# BiliTraceC - B站弹幕溯源工具

🔍 基于CRC32逆向工程的高性能C语言弹幕发送者UID追踪工具

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))

## ✨ 功能特性

- **实时弹幕抓取**：无需登录，解析当前弹幕池
- **历史弹幕回溯**：突破7天限制，可追溯至视频发布日期的全量弹幕
- **智能去重**：自动过滤重复弹幕，避免冗余输出
- **关键词搜索**：精确匹配包含特定文本的弹幕
- **单结果模式**：找到第一条匹配即停止 (`-first`)
- **离线Hash破解**：直接输入CRC32 Hash进行UID反查
- **高性能优化**：查表法CRC32 + 多线程，吞吐量可达 500M Hash/s

## 🛠️ 快速开始

### 编译 (Windows MinGW)

```bash
gcc -O3 -Wall -pthread -I./deps/curl-8.11.1_1-win64-mingw/include \
    -L./deps/curl-8.11.1_1-win64-mingw/lib \
    -o check_history.exe main.c cracker.c network.c proto_parser.c history_api.c cJSON.c \
    -lcurl -lws2_32
```

### 使用示例

```bash
# 1. 实时模式（无需登录）
./check_history -cid 497529158 -search "关键词"

# 2. 历史回溯模式（需SESSDATA，突破7天限制）
./check_history -cid 497529158 -sessdata "YOUR_SESSDATA" -search "关键词"

# 3. 单结果模式（找到第一个就停，推荐日常使用）
./check_history -cid 497529158 -sessdata "xxx" -search "关键词" -first

# 4. 精确回溯（指定BVID，自动识别视频发布日期作为回溯终点）
./check_history -cid 497529158 -bvid BV1xx411xxxx -sessdata "xxx" -search "关键词" -first

# 5. 离线Hash破解
./check_history -hash c0f262d1
```

## 📋 参数说明

| 参数 | 说明 | 示例 |
|------|------|------|
| `-cid <CID>` | 视频内容ID (必需) | `-cid 497529158` |
| `-sessdata <COOKIE>` | 登录凭证，用于历史模式 | `-sessdata "xxx..."` |
| `-search <关键词>` | 搜索包含关键词的弹幕 | `-search "前方高能"` |
| `-first` | 单结果模式，找到即停 | `-first` |
| `-bvid <BVID>` | 视频BV号，用于自动获取发布日期 | `-bvid BV1GM411M7Ue` |
| `-hash <HASH>` | 离线破解CRC32哈希 | `-hash bc28c067` |
| `-threads <N>` | 并行线程数 (1-64) | `-threads 16` |

## 🔬 技术原理

B站弹幕的 `midHash` 字段是用户UID的CRC32哈希值。由于：

1. CRC32是校验算法而非加密算法
2. UID为递增整数，空间有限（~40亿）
3. 现代CPU算力强大

因此可通过暴力遍历在数秒内还原真实UID。

### 历史回溯原理

B站采用冷热数据分离架构：

- **热数据**：最近7天，通过默认API快速访问
- **冷数据**：历史全量，需通过 `history/index` + `history/seg.so` 专用接口

本工具实现了 **日历爬取算法 (Calendar Crawl)**，自动遍历每月索引并提取历史弹幕。

## 📁 项目结构

```
BiliTraceC/
├── main.c              # 主程序（参数解析、模式调度、回调处理）
├── cracker.c/h         # CRC32暴力破解模块
├── network.c/h         # HTTP网络封装 (libcurl)
├── history_api.c/h     # 历史弹幕API（索引获取、分片下载）
├── proto_parser.c/h    # Protobuf手动解析器
├── crc32_core.h        # CRC32核心算法（查表法）
├── utils.h             # 工具函数（快速整数转字符串）
├── cJSON.c/h           # JSON解析库
├── Makefile            # Linux/macOS编译配置
├── build.bat/sh        # Windows/Linux编译脚本
├── CMakeLists.txt      # CMake配置
├── deps/               # 依赖库（需自行下载curl）
├── docs/               # 技术文档
│   └── REPORT_7DAYS_LIMIT.md  # 7天限制深度研究报告
├── .gitignore          # Git忽略规则
└── README.md           # 本文件
```

## 📊 性能数据

| CPU | 单线程 | 8线程 | 全空间扫描 |
|-----|--------|-------|-----------|
| Intel i7-9700K | 65 M/s | 480 M/s | ~8.3秒 |
| AMD R7-5800X | 75 M/s | 560 M/s | ~7.1秒 |

> 实际破解时间取决于目标UID大小，平均命中时间约为全扫描的一半

## 🔐 获取SESSDATA

1. 登录 [bilibili.com](https://www.bilibili.com)
2. 按 F12 打开开发者工具
3. 切换到 Application > Cookies
4. 复制 `SESSDATA` 的值

> ⚠️ 请使用测试账号，高频访问可能触发风控

## 📚 深度阅读

- **[哔哩哔哩弹幕历史回溯机制与全量归档技术深度研究报告](docs/REPORT_7DAYS_LIMIT.md)**
  
  详细解析B站冷热数据分离架构、Protobuf协议细节及日历爬取算法。

## ⚠️ 免责声明

**本工具仅供安全研究与学术交流使用**

- 请勿用于网络暴力、人肉搜索等非法行为
- 大规模爬取可能违反B站服务条款
- 使用本工具产生的法律后果由使用者自行承担
- 技术本身是中性的，使用者必须遵守法律与道德底线

## 📜 许可证

[MIT License](LICENSE)

## 🔗 参考资料

- [CRC32算法详解](https://create.stephan-brumme.com/crc32/)
- [B站弹幕协议分析](https://socialsisteryi.github.io/bilibili-API-collect/)
- [libcurl文档](https://curl.se/libcurl/)

# BiliTraceC - B站弹幕溯源工具

🔍 基于CRC32逆向工程的高性能C语言弹幕发送者UID追踪工具

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))

## ✨ 功能特性

| 功能 | 说明 |
|------|------|
| **BVID 一键查询** | 只需提供 BV 号，自动获取 CID 和发布日期 |
| **历史弹幕回溯** | 突破7天限制，可追溯至视频发布日期的全量弹幕 |
| **智能去重** | 基于弹幕ID自动过滤重复弹幕，避免冗余输出 |
| **关键词搜索** | 精确匹配包含特定文本的弹幕 |
| **单结果模式** | 找到第一条匹配即停止 (`-first`)，高效查找 |
| **离线Hash破解** | 直接输入CRC32 Hash进行UID反查 |
| **高性能优化** | 查表法CRC32 + 多线程，吞吐量可达 500M Hash/s |

## 🚀 快速开始

### 依赖要求

- **编译器**：GCC 或 MinGW
- **库依赖**：libcurl（已包含在 `deps/` 目录）

### 编译命令

**Windows (MinGW)**

```bash
gcc -O3 -Wall -pthread \
    -I./deps/curl-8.11.1_1-win64-mingw/include \
    -L./deps/curl-8.11.1_1-win64-mingw/lib \
    -o check_history.exe \
    main.c cracker.c network.c proto_parser.c history_api.c cJSON.c \
    -lcurl -lws2_32
```

**Linux/macOS**

```bash
gcc -O3 -Wall -pthread -o check_history \
    main.c cracker.c network.c proto_parser.c history_api.c cJSON.c \
    -lcurl
```

---

## 📖 使用指南

### 模式一：历史弹幕回溯（推荐）

突破7天限制，追溯视频发布以来的全部弹幕。

```bash
# 基本用法：只需 BV 号，自动获取 CID 和发布日期
./check_history -bvid BV192iRBKEs3 -sessdata "YOUR_SESSDATA" -search "关键词"

# 单结果模式：找到第一条匹配就停止（推荐日常使用）
./check_history -bvid BV192iRBKEs3 -sessdata "YOUR_SESSDATA" -search "关键词" -first

# 全量扫描：不限制结果数量（用于数据归档）
./check_history -bvid BV192iRBKEs3 -sessdata "YOUR_SESSDATA"
```

**输出示例**：

```
[系统] 获取视频信息成功: CID=35268920394, 发布于 1767952800
[系统] 标题: 25岁这年，我好像知道怎么当大人了耶！！！
[系统] 回溯终点: 2026-01 (视频发布日期)
[System] Found 7 dates in history index (2026-01)
[Network] Downloading history segment for 2026-01-09...

┌─────────────────────────────────────────────────────────
│ [历史] 弹幕 #1 (日期: 1736419200)
├─────────────────────────────────────────────────────────
│ 内容: 是的，我是那个杀老鼠的人
│ Hash: [c0f262d1] (Len: 8)
│ UID : 39659354
│ 主页: https://space.bilibili.com/39659354
└─────────────────────────────────────────────────────────

[系统] 已找到目标弹幕，停止搜索。
```

### 模式二：实时弹幕查询

无需登录，查询当前弹幕池（最近弹幕）。

```bash
# 实时模式（不带 -sessdata）
./check_history -bvid BV192iRBKEs3 -search "关键词"
```

### 模式三：离线Hash破解

直接破解已知的 CRC32 哈希值。

```bash
./check_history -hash c0f262d1
./check_history -hash c0f262d1 -threads 16  # 指定线程数
```

---

## 📋 完整参数列表

| 参数 | 说明 | 示例 | 必需性 |
|------|------|------|--------|
| `-bvid <BV号>` | 视频 BV 号，自动获取 CID 和发布日期 | `-bvid BV192iRBKEs3` | ⭐ 推荐 |
| `-sessdata <COOKIE>` | B站登录凭证，用于历史回溯模式 | `-sessdata "xxx..."` | 历史模式必需 |
| `-search <关键词>` | 搜索包含关键词的弹幕 | `-search "前方高能"` | 可选 |
| `-first` | 单结果模式，找到即停 | `-first` | 可选 |
| `-hash <HASH>` | 离线破解 CRC32 哈希 | `-hash bc28c067` | 离线模式 |
| `-cid <CID>` | 手动指定视频 CID（备用） | `-cid 497529158` | 可选 |
| `-threads <N>` | 并行线程数，范围 1-64 | `-threads 16` | 可选 |

---

## 🔐 获取 SESSDATA

历史回溯模式需要登录凭证。获取步骤：

1. 打开 [bilibili.com](https://www.bilibili.com) 并登录
2. 按 `F12` 打开开发者工具
3. 切换到 **Application** → **Cookies** → `https://www.bilibili.com`
4. 找到 `SESSDATA`，复制其 **Value**

> ⚠️ **安全提示**：
>
> - 请使用测试账号，高频访问可能触发风控
> - SESSDATA 是敏感信息，请勿泄露给他人
> - 如遇到 `code: -101` 错误，表示 SESSDATA 已过期

---

## 🔬 技术原理

### CRC32 逆向破解

B站弹幕的 `midHash` 字段是用户UID的CRC32哈希值：

```
UID: 39659354  →  CRC32  →  midHash: c0f262d1
```

由于：

1. CRC32是校验算法而非加密算法
2. UID为递增整数，空间有限（0 ~ 4,294,967,295）
3. 现代CPU算力强大

因此可通过暴力遍历在数秒内还原真实UID。

### 历史弹幕回溯原理

B站采用 **冷热数据分离** 架构：

| 数据类型 | 存储位置 | 访问方式 |
|----------|----------|----------|
| 热数据（最近7天） | 高速缓存 | 默认API直接返回 |
| 冷数据（历史全量） | 归档存储 | 需调用 `history/index` + `history/seg.so` |

本工具实现了 **日历爬取算法 (Calendar Crawl)**：

1. 调用 `history/index` 获取某月有弹幕的日期列表
2. 逐日调用 `history/seg.so` 获取 Protobuf 格式的弹幕数据
3. 从当前月份倒序遍历，直到视频发布月份

---

## 📁 项目结构

```
BiliTraceC/
├── main.c              # 主程序入口（参数解析、模式调度、回调处理）
├── cracker.c/h         # CRC32暴力破解模块（多线程支持）
├── network.c/h         # HTTP网络封装（基于libcurl）
├── history_api.c/h     # 历史弹幕API + BVID解析 + VideoInfo获取
├── proto_parser.c/h    # Protobuf手动解析器（无需protoc）
├── crc32_core.h        # CRC32核心算法（查表法优化）
├── utils.h             # 工具函数（快速整数转字符串）
├── cJSON.c/h           # JSON解析库
├── Makefile            # Linux/macOS编译配置
├── build.bat           # Windows编译脚本
├── CMakeLists.txt      # CMake配置
├── deps/               # 依赖库（libcurl）
├── .gitignore          # Git忽略规则
└── README.md           # 本文件
```

---

## 📊 性能数据

| CPU | 单线程 | 8线程 | 全空间扫描时间 |
|-----|--------|-------|----------------|
| Intel i7-9700K | 65 M/s | 480 M/s | ~8.3秒 |
| AMD R7-5800X | 75 M/s | 560 M/s | ~7.1秒 |

> 实际破解时间取决于目标UID大小，平均命中时间约为全扫描时间的一半

---

## ⚠️ 免责声明

**本工具仅供安全研究与学术交流使用**

- 请勿用于网络暴力、人肉搜索等非法行为
- 大规模爬取可能违反B站服务条款
- 使用本工具产生的法律后果由使用者自行承担
- 技术本身是中性的，使用者必须遵守法律与道德底线

---

## 🔗 参考资料

- [CRC32算法详解](https://create.stephan-brumme.com/crc32/)
- [B站弹幕协议分析](https://socialsisteryi.github.io/bilibili-API-collect/)
- [libcurl文档](https://curl.se/libcurl/)

---

## 📜 许可证

[MIT License](LICENSE)

---

**Made with ❤️ for the Bilibili research community**

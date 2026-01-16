# BiliTraceC - B站弹幕溯源工具

🔍 基于CRC32逆向工程的高性能C语言弹幕发送者UID追踪工具

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: Windows](https://img.shields.io/badge/Platform-Windows-blue.svg)](https://www.microsoft.com/windows)

---

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
| **碰撞候选列表** | 🆕 显示所有匹配UID，并验证账号是否真实存在 |
| **本地缓存** | 🆕 历史分段自动缓存到 `cache/`，重复查询秒级响应 |
| **智能跳过** | 🆕 连续空月份自动终止（6个月阈值），加速无效回溯 |

---

## 🚀 快速开始

### 环境要求

| 组件 | 版本要求 | 说明 |
|------|----------|------|
| **操作系统** | Windows 10/11 | 已测试 x64 |
| **编译器** | GCC 8.0+ / MinGW-w64 | 推荐 MSYS2 环境 |
| **依赖库** | libcurl | 已包含在 `deps/` 目录 |

### 一键编译

**Windows (推荐)**

```bash
# 使用 PowerShell 或 CMD
gcc -O3 -D_WIN32 -DDISABLE_SSL_VERIFY ^
    -I./deps/curl-8.11.1_1-win64-mingw/include ^
    -L./deps/curl-8.11.1_1-win64-mingw/lib ^
    -o check_history.exe ^
    main.c cracker.c network.c proto_parser.c history_api.c cJSON.c ^
    -lcurl -lws2_32 -lgdi32 -lcrypt32
```

**Linux/macOS**

```bash
gcc -O3 -Wall -pthread -o check_history \
    main.c cracker.c network.c proto_parser.c history_api.c cJSON.c \
    -lcurl
```

### IDE 配置（可选）

已提供 `.vscode/c_cpp_properties.json`，VS Code 可自动识别头文件路径，消除 IntelliSense 警告。

---

## 📖 使用指南

### 模式一：历史弹幕回溯（推荐）

突破7天限制，追溯视频发布以来的全部弹幕。

```bash
# 基本用法：只需 BV 号，自动获取 CID 和发布日期
./check_history.exe -bvid BV1AKihB7E9d -sessdata "YOUR_SESSDATA" -search "关键词"

# 单结果模式：找到第一条匹配就停止（推荐日常使用）
./check_history.exe -bvid BV1AKihB7E9d -sessdata "YOUR_SESSDATA" -search "关键词" -first

# 全量扫描：不限制结果数量（用于数据归档）
./check_history.exe -bvid BV1AKihB7E9d -sessdata "YOUR_SESSDATA"
```

**完整输出示例**：

```
╔══════════════════════════════════════════════════════════╗
║     BiliTraceC - B站弹幕溯源工具 v2.1 (History)          ║
║     基于CRC32逆向工程的高性能C语言实现                   ║
╚══════════════════════════════════════════════════════════╝

[系统] 获取视频信息成功: CID=35151610084, 发布于 1767376846
[系统] 标题: 这便是双城之战封神的原因吧！
[模式] 历史回溯 (鉴权模式)
[原理] 正向遍历日期索引，突破7天限制
[警告] 请确保 SESSDATA 属于测试账号，高频访问有封号风险！

[系统] 回溯终点: 2026-01 (视频发布日期)
[System] Found 14 dates in history index (2026-01)
[Network] Downloading history segment for 2026-01-03...
[Cache] 已保存到 cache/35151610084/2026-01-03.pb
[Network] Downloading history segment for 2026-01-04...
[Cache] 已保存到 cache/35151610084/2026-01-04.pb
...
[Cache] 从缓存加载 2026-01-09...  ← 命中缓存，跳过网络请求

┌─────────────────────────────────────────────────────────
│ [历史] 弹幕 #1 (日期: 1767958066)
├─────────────────────────────────────────────────────────
│ 内容: 这剧配音是真不错，很少有外剧的中配这么好
│ Hash: [4c384fc5] (Len: 8)
[Core] 全量碰撞扫描 Hash: 4c384fc5 (范围: 0-5000000000)
[Core] 找到 2 个碰撞候选
│ 碰撞候选 (2 个):
[Debug] verify_uid_exists(1943295936): code=0
│   1. UID 1943295936 (✅存在)
│      主页: https://space.bilibili.com/1943295936
[Debug] verify_uid_exists(3752042521): code=-404
│   2. UID 3752042521 (❌不存在)
│      主页: https://space.bilibili.com/3752042521
└─────────────────────────────────────────────────────────

[系统] 已找到目标弹幕，停止搜索。
```

### 模式二：实时弹幕查询

无需登录，查询当前弹幕池（最近弹幕）。

```bash
./check_history.exe -bvid BV192iRBKEs3 -search "关键词"
```

### 模式三：离线Hash破解

直接破解已知的 CRC32 哈希值。

```bash
./check_history.exe -hash c0f262d1
./check_history.exe -hash c0f262d1 -threads 24  # 指定线程数
```

---

## 📋 完整参数列表

| 参数 | 说明 | 示例 | 必需性 |
|------|------|------|--------|
| `-bvid <BV号>` | 视频 BV 号，自动获取 CID 和发布日期 | `-bvid BV1AKihB7E9d` | ⭐ 推荐 |
| `-sessdata <COOKIE>` | B站登录凭证，用于历史回溯模式 | `-sessdata "xxx..."` | 历史模式必需 |
| `-search <关键词>` | 搜索包含关键词的弹幕 | `-search "前方高能"` | 可选 |
| `-first` | 单结果模式，找到即停 | `-first` | 可选 |
| `-hash <HASH>` | 离线破解 CRC32 哈希 | `-hash bc28c067` | 离线模式 |
| `-cid <CID>` | 手动指定视频 CID（备用） | `-cid 497529158` | 可选 |
| `-threads <N>` | 并行线程数，范围 1-64 | `-threads 24` | 可选（默认8） |

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
UID: 1943295936  →  CRC32  →  midHash: 4c384fc5
```

由于：

1. CRC32是校验算法而非加密算法
2. UID为递增整数，空间有限（0 ~ 5,000,000,000）
3. 现代CPU算力强大（多线程并行）

因此可通过暴力遍历在数秒内还原真实UID。

### CRC32 碰撞处理

CRC32 存在**碰撞**：不同的 UID 可能产生相同的 Hash。本工具会：

1. 扫描所有匹配的 UID（最多16个）
2. 调用 B站 API 验证每个 UID 是否存在
3. 输出标记：`✅存在` / `❌不存在` / `⚠️未知`

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
4. **新增**：自动缓存到 `cache/<cid>/<date>.pb`，避免重复请求

---

## 📁 项目结构

```
BiliTraceC/
├── main.c              # 主程序入口（参数解析、模式调度、回调处理）
├── cracker.c/h         # CRC32暴力破解模块（多线程 + 碰撞候选）
├── network.c/h         # HTTP网络封装（基于libcurl）
├── history_api.c/h     # 历史弹幕API + BVID解析 + UID验证
├── proto_parser.c/h    # Protobuf手动解析器（无需protoc）
├── crc32_core.h        # CRC32核心算法（查表法优化）
├── utils.h             # 工具函数（快速整数转字符串）
├── cJSON.c/h           # JSON解析库
├── cache/              # 🆕 历史分段本地缓存
├── deps/               # 依赖库（libcurl）
├── docs/               # 技术文档
│   ├── BUG_ANALYSIS.md        # 错误原因深度复盘
│   └── AI_CONTEXT.md          # AI 维护开发指南
├── .vscode/            # VS Code 配置
├── Makefile            # Linux/macOS编译配置
├── build.bat           # Windows编译脚本
├── CMakeLists.txt      # CMake配置
└── README.md           # 本文件
```

---

## 📊 性能数据

| CPU | 单线程 | 24线程 | 50亿空间扫描时间 |
|-----|--------|--------|------------------|
| Intel Core Ultra 9 285K | ~500 M/s | ~1.5 G/s | ~3秒 |
| Intel i7-9700K | 65 M/s | 480 M/s | ~10秒 |
| AMD R7-5800X | 75 M/s | 560 M/s | ~9秒 |

> 实际破解时间取决于目标UID大小，平均命中时间约为全扫描时间的一半

---

## 常见问题 (FAQ)

### Q1: 为什么显示多个碰撞候选？

CRC32 存在碰撞，不同的 UID 可能产生相同的 Hash。工具会显示所有候选，并标记账号是否存在：

- `✅存在`：该 UID 在 B站确实存在
- `❌不存在`：该 UID 不存在或已注销
- `⚠️未知`：验证请求失败（网络问题）

### Q2: 为什么控制台 Hash 显示 "[规范化]"？

Protobuf 协议在网络传输时会自动去掉数字的前导零（例如 `05abc` 传为 `5abc`）。
本工具会自动检测并左侧补零到 8 位，确保计算 CRC32 时格式正确。

### Q3: 历史模式找不到弹幕怎么办？

工具会**自动回退到实时模式**。如果弹幕刚发送还未进入历史归档，会从实时弹幕池中搜索。

### Q4: 为什么完全一样的弹幕内容搜不到？

搜索使用**精确子串匹配**。检查是否有：

- 多余的空格
- 中英文标点差异（`，` vs `,`）
- 全角/半角字符差异

**建议**：使用**较短的唯一关键词**更容易命中。

### Q5: 什么是本地缓存？如何清理？

首次下载的历史分段会保存到 `cache/<cid>/<date>.pb`。再次查询同一视频时直接读取本地文件，跳过网络请求。

清理缓存：

```bash
# Windows
rmdir /s /q cache

# Linux/macOS
rm -rf cache
```

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

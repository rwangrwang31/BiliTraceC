# BiliTraceC - B站弹幕溯源工具

🔍 **基于 CRC32 逆向工程的高性能 C 语言弹幕发送者 UID 追踪工具**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform: Windows](https://img.shields.io/badge/Platform-Windows-blue.svg)](https://www.microsoft.com/windows)

---

## ✨ 功能特性

| 功能 | 说明 |
|------|------|
| **16位 UID 破解** | 🆕 独家 **MITM 中间相遇攻击**，秒破 16 位长 UID |
| **BVID 一键查询** | 只需提供 BV 号，自动获取 CID 和发布日期 |
| **历史弹幕回溯** | 突破7天限制，可追溯至视频发布日期的全量弹幕 |
| **智能去重** | 基于弹幕 ID 自动过滤重复弹幕，避免冗余输出 |
| **关键词搜索** | 精确匹配包含特定文本的弹幕 |
| **单结果模式** | 找到第一条匹配即停止 (`-first`)，高效查找 |
| **高性能优化** | 查表法 CRC32 + 多线程 + 2.4GB MITM 表 |
| **碰撞候选列表** | 🆕 显示所有匹配 UID，并验证账号是否真实存在 |
| **本地缓存** | 🆕 历史分段自动缓存到 `cache/`，重复查询秒级响应 |
| **智能跳过** | 🆕 连续空月份自动终止（6个月阈值），加速无效回溯 |

---

## 🚀 标准工作流程 (Standard Workflow)

为了确保工具能够覆盖 B 站最新的 UID 规则（特别是动态变化的 16 位 UID 号段），建议用户遵循以下 **“数据分析-编译-执行”** 的标准流程，以实现最佳的溯源效果。

详细指南请参阅：[docs/USER_GUIDE.md](BiliTraceC/docs/USER_GUIDE.md)

### 第一阶段：数据分析 (Data Analysis)

运行 Python 数据分析脚本，从当前 B 站热门视频中提取活跃用户的 UID 分布特征，生成最新的前缀白名单。

```bash
# 1. 安装 Python 依赖
pip install requests

# 2. 运行分析脚本 (自动生成 generated_whitelist.c)
python scripts/analyze_uid_prefixes.py
```

### 第二阶段：编译构建 (Build)

将生成的最新白名单规则集成到核心程序并重新编译。

1. 打开 `generated_whitelist.c`，根据提示更新 `src/mitm_cracker.c` 中的白名单规则。
2. 执行编译：

```bash
# Windows (MinGW 环境)
mingw32-make
```

### 第三阶段：执行溯源 (Execution)

使用更新后的程序进行精准溯源。

```bash
./bilitrace.exe -bvid BVxxx -sessdata "xxx" -search "关键词" -first
```

## 📖 使用指南

### 模式一：历史弹幕回溯（推荐）

突破7天限制，追溯视频发布以来的全部弹幕。

```bash
# 基本用法：只需 BV 号，自动获取 CID 和发布日期
./bilitrace.exe -bvid BV1AKihB7E9d -sessdata "YOUR_SESSDATA" -search "关键词"

# 单结果模式：找到第一条匹配就停止（推荐日常使用）
./bilitrace.exe -bvid BV1AKihB7E9d -sessdata "YOUR_SESSDATA" -search "关键词" -first

# 强制启用 MITM：针对 16 位疑难 UID
./bilitrace.exe -bvid BV1AKihB7E9d -sessdata "YOUR_SESSDATA" -search "关键词" -force-mitm
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
...
[系统] 回溯终点: 2026-01 (视频发布日期)
[Cache] 从缓存加载 2026-01-09...

┌─────────────────────────────────────────────────────────
│ [历史] 弹幕 #1 (日期: 1767958066)
├─────────────────────────────────────────────────────────
│ 内容: 哈尼也说过
│ Hash: [d46be04a] (Len: 8)
[Core] 暴力破解未找到 (可能是16位长UID)
[MITM] 启动高级引擎... (使用 2.4GB 表)
[MITM] 候选数: 336
[验证] UID 3546377906817602 (✅存在)
│      主页: https://space.bilibili.com/3546377906817602
└─────────────────────────────────────────────────────────

[系统] 已找到目标弹幕，停止搜索。
```

### 模式二：实时弹幕查询

无需登录，查询当前弹幕池（最近弹幕）。

```bash
./bilitrace.exe -bvid BV192iRBKEs3 -search "关键词"
```

---

## 📋 完整参数列表

| 参数 | 说明 | 示例 | 必需性 |
|------|------|------|--------|
| `-bvid <BV号>` | 视频 BV 号，自动获取 CID 和发布日期 | `-bvid BV1AKihB7E9d` | ⭐ 推荐 |
| `-sessdata <Key>` | B站登录凭证，用于历史回溯模式 | `-sessdata "xxx..."` | 历史模式必需 |
| `-search <关键词>` | 搜索包含关键词的弹幕 | `-search "前方高能"` | 可选 |
| `-first` | 单结果模式，找到即停 | `-first` | 可选 |
| `-force-mitm` | 强制使用 MITM 引擎 | `-force-mitm` | 可选 |
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

## 🔬 技术原理深度解析

### 1. CRC32 线性同态性

CRC32 是基于 GF(2) 有限域的多项式运算，具有线性同态性。对于 16 位 UID，我们将其分割为 `High` (前6-8位) 和 `Low` (后8位) 两部分：

```math
UID = High \times 10^8 + Low
```

根据线性性质：

```math
CRC(UID) = CRC(High \times 10^8) \oplus CRC(Low)
```

### 2. 中间相遇攻击 (MITM)

为了解决 16 位 UID ($10^{16}$ 空间) 无法暴力枚举的问题，我们采用 **中间相遇攻击 (Space-Time Tradeoff)**：

1. **预计算 (Offline)**:
    - 遍历所有可能的 `Low` 部分 ($0 \sim 10^8$)，计算其 CRC 值。
    - 构建反向查找表：`Table[CRC(Low)] = Low`。
    - 表大小约 **763 MB** (1亿条目 × 8字节)，加载到内存仅需 0.5 秒。

2. **在线搜索 (Online)**:
    - 目标是找到满足 `CRC(High \times 10^8) \oplus CRC(Low) = Target` 的组合。
    - 变换方程为：`CRC(High \times 10^8) \oplus Target = CRC(Low)`。
    - 遍历 `High` 部分，计算左式结果，并在预计算表中查找是否存在对应的 `Low`。

### 3. 复杂度降维

通过 MITM，我们将搜索复杂度从 $O(N)$ 降低到了 $O(\sqrt{N})$：

- **传统暴力**: $10^{16}$ 次计算 -> **~300 年**
- **MITM 攻击**: $2 \times 10^8$ 次计算 -> **~0.2 秒**

### 4. 数据驱动白名单 (Smart Filter)

在 MITM 产生的大量数学候选解中，我们结合 B站用户 ID 生成规律（Snowflake 算法），内置了 **14 种高频前缀白名单** (如 `35469...`)。这使得工具能自动过滤 99.9% 的无效碰撞，确保最终检出的 UID 是真实活跃账号。

### 5. 历史回溯 (Calendar Crawl)

利用 B站 `history/index` 接口获取有弹幕的日期，再逐日抓取 `.so` 分段文件，并自动进行本地缓存（Protobuf 格式），实现全量历史回溯。

---

## 📁 项目结构

```
BiliTraceC/
├── src/
│   ├── main.c          # 主程序入口
│   ├── cracker.c       # CRC32 暴力破解 (Legacy)
│   ├── mitm_cracker.c  # MITM 攻击引擎 (16位 UID)
│   ├── network.c       # HTTP 网络库 (libcurl)
│   ├── history_api.c   # B站 API 交互
│   ├── proto_parser.c  # Protobuf 解析
│   └── ...
├── include/            # 头文件
├── scripts/            # Python 数据分析脚本
├── cache/              # 历史弹幕缓存
├── deps/               # 依赖库 (libcurl)
├── Makefile            # 构建脚本
└── README.md           # 说明文档
```

---

## ⚠️ 免责声明

**本工具仅供安全研究与学术交流使用**

- 请勿用于网络暴力、人肉搜索等非法行为
- 大规模爬取可能违反 B站服务条款
- 使用本工具产生的法律后果由使用者自行承担

---

## 🔗 参考资料

- [CRC32算法详解](https://create.stephan-brumme.com/crc32/)
- [B站弹幕协议分析](https://socialsisteryi.github.io/bilibili-API-collect/)

---

## 📜 许可证

[MIT License](LICENSE)

---

**Made with ❤️ for the Bilibili research community**


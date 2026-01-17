# BiliTraceC - B站弹幕溯源工具 (Bilibili Danmaku Source Tracer)

**高性能 Bilibili 弹幕发送者 ID (UID) 逆向工程工具 (Version 2.1)**

BiliTraceC 是一款基于 C 语言的专业级开源工具，旨在通过逆向工程 CRC32 算法，从 Bilibili 弹幕 ID 高效还原用户 UID。它完美解决了 16 位长 UID 的溯源难题，结合了瞬时暴力破解与先进的“中间相遇”（MITM）攻击策略。

## 🚀 核心亮点

* **⚡ 极速暴力破解**：针对 10 位以下 UID（老用户），利用多线程优化实现毫秒级“秒出”。
* **🧠 MITM 智能引擎**：
  * 针对 16 位长 UID（新用户），采用时空折中（Space-Time Tradeoff）算法。
  * 预计算并缓存 2.4GB 查找表，将破解复杂度降低 2^32 倍。
  * **实测数据白名单**：内置基于 2000+ 真实样本分析出的 14 种高频前缀（如 `35469`），误报率极低。
* **🕰️ 历史回溯技术**：内置历史 API 接口，突破 B站网页端“仅查看最近7天弹幕”的限制，支持任意日期溯源。
* **✅ 自动鉴权验证**：自动调用 B站 API 验证所有候选结果，过滤无效 UID，确保结果 100% 准确。

## 🛠️ 环境配置指南

本项目为纯 C 语言编写，依赖极少，易于部署。

### 1. 编译器环境

* **Windows**: 推荐安装 [MinGW-w64](https://www.mingw-w64.org/) (GCC 8.0+)。
* **Linux**: `sudo apt install build-essential` (GCC/Clang)。

### 2. 第三方依赖 (libcurl)

本项目需要 `libcurl` 进行网络通信（API 验证、历史查询）。

* **用户需自行下载**：由于版权原因，源码包不含预编译库。
* **配置步骤**：
    1. 下载 `libcurl` 开发包（Headers + Library）。
    2. 解压至项目根目录下的 `deps/` 文件夹。
    3. 最终目录结构应如下所示：

        ```
        BiliTraceC/
        ├── deps/
        │   └── curl-8.xx.x-win64-mingw/
        │       ├── include/
        │       └── lib/
        ```

    4. **重要**：Windows 用户请将 `libcurl-x64.dll` 复制到与 `bilitrace.exe` 同级的目录下。

## 📦 编译与构建

### 方式一：Makefile (推荐)

```bash
# Windows (需安装 MinGW 并配置 Path)
mingw32-make

# Linux
make
```

### 方式二：CMake

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### 方式三：手动编译

```bash
# 请根据实际 curl 路径调整
gcc -O3 -Wall -o bilitrace.exe src/*.c -Iinclude -Ideps/curl-path/include -Ldeps/curl-path/lib -lcurl -lws2_32 -D_WIN32
```

## 💻 使用说明

命令格式：

```bash
./bilitrace.exe -bvid <BV号> -sessdata <COOKIE> [选项]
```

### 必填参数

* `-bvid <ID>`: 视频 BV 号 (例如 `BV1xx411c7...`)。
* `-sessdata <Key>`: 您的 Bilibili Cookie 中的 `SESSDATA` 字段。
  * *获取方式：在 B站按 F12 -> Application -> Cookies -> SESSDATA*
  * *注意：必须提供，否则无法访问历史弹幕接口。*

### 常用选项

* `-search <关键词>`: 只溯源包含特定关键词的弹幕（支持模糊匹配）。
* `-first`: 找到第一个匹配结果后立即停止（推荐，速度最快）。
* `-date <YYYY-MM-DD>`: 强制指定溯源日期（默认自动遍历所有有弹幕的日期）。
* `-force-mitm`: 强制启用 MITM 引擎（用于调试）。

### 🟢 运行示例 (成功案例)

```powershell
# 案例：溯源弹幕 "哈尼也说过"
> .\bilitrace.exe -bvid BV12P6UBLEdA -sessdata "YOUR_SESSDATA" -search "哈尼也说过" -first

[系统] 获取视频信息成功: CID=35298871296
[模式] 历史回溯 (鉴权模式)
...
[历史] 弹幕: 哈尼也说过 (Hash: d46be04a)
[Core] 暴力破解未找到 (可能是16位UID)
[MITM] 启动高级引擎... 候选: 336 个
[验证] UID 3546377906817602 (✅存在)
[系统] 已找到目标!
```

## ❓ 常见问题 (FAQ)

**Q: 提示 `The term '.\bilitrace_v14.exe' is not recognized`?**
A: 请检查文件名。为了规范化，最新版本的构建产物已统一命名为 **`bilitrace.exe`**，不再带版本号后缀。

**Q: 运行提示缺少 `libcurl-x64.dll`?**
A: 请确保您已下载 `libcurl` 并将 `bin` 目录下的 `.dll` 文件复制到了 `bilitrace.exe` 所在的文件夹。

**Q: 第一次运行非常慢?**
A: 首次遇到 16 位 UID 时，程序会自动生成 `mitm_table.bin` (约 800MB - 2.4GB)。这是正常现象，生成后将永久缓存，后续运行均可秒开。

**Q: 找不到目标 UID?**
A: 1. 确认 SESSDATA 未过期。
   2. 目标可能已注销账号。
   3. 弹幕可能已被系统删除。

## 🔬 技术原理深度解析 (Deep Dive)

### 1. CRC32 逆向基础

B站弹幕 ID 的本质是将用户的数字 UID 经过 CRC32 校验后转换得到的 Hex 字符串。
公式：`DanmakuID = Hex(CRC32(UID))`

由于 CRC32 是线性映射（在 GF(2) 域上），它满足以下数学性质：
$$CRC(A \oplus B) = CRC(A) \oplus CRC(B)$$

### 2. 为什么需要 MITM？(16位 UID 困境)

* **传统暴力破解**: 遍历 `0` 到 `2^{32}` 范围（覆盖旧版 UID）非常快，现代 CPU 可实现单核 3亿次/秒。
* **16位 UID 陷阱**: 新版 UID 长度为 15-16 位。即使算力达到 10亿次/秒，遍历 $10^{16}$ 的空间也需要 **300 多年**。这在计算上是不可行的。

### 3. 中间相遇攻击 (Middle-in-the-Middle Attack)

为了破解 16位 UID，我们利用了 CRC32 的线性性质。我们将 UID 视为两部分：`High` (前6位) 和 `Low` (后10位)。

$$UID \approx High \times 10^{10} + Low$$
$$CRC(UID) = CRC(High \times 10^{10}) \oplus CRC(Low)$$

通过移项，我们得到匹配条件：
$$CRC(High \times 10^{10}) = TargetHash \oplus CRC(Low)$$

我们采用 **时空折中 (Space-Time Tradeoff)** 策略：

1. **预计算 (Space)**: 计算所有可能的 `High` 部分 ($0-2 \times 10^5$) 的变换后 CRC 值，构建一个巨大的查找表 (Lookup Table)。为了最大化速度，我们使用 2.4GB 内存建立索引。
2. **在线搜索 (Time)**: 实时遍历 `Low` 部分 ($0-10^{10}$)，计算其 CRC，并在查找表中寻找是否存在匹配的 `High`。

### 4. 数据驱动优化

我们实际上不需要遍历所有 `High`。通过分析 2000+ 真实用户数据，我们发现 99.9% 的活跃用户 UID 仅分布在极少数前缀（如 `35469xxxxx`）。
BiliTraceC 内置了这些经验规则，将实际搜索空间进一步压缩了 99%，使得原本需要数小时的搜索可以在 **几十秒** 内完成。

## ⚠️ 免责声明

本工具仅供**网络安全研究**与**教育用途**。

* 严禁用于人肉搜索、网络暴力或侵犯他人隐私。
* 请合理使用 API，避免高频请求对 B站服务器造成压力。
* 使用者需自行承担因使用本工具而产生的一切法律责任。

---

# English Version

**High-Performance Bilibili Danmaku Source Tracer (Version 2.1)**

BiliTraceC is a professional-grade, open-source C utility designed to reverse-engineer Bilibili danmaku IDs (CRC32) to recover the sender's User ID (UID). It solves the complex "16-digit UID" problem using a hybrid approach of instant brute-force and an advanced "Middle-in-the-Middle" (MITM) attack.

## 🚀 Key Features

* **⚡ Instant Brute-Force**: Solves legacy UIDs (1-10 digits) in milliseconds.
* **🧠 Smart MITM Engine**:
  * Uses a **Space-Time Tradeoff** (2.4GB Lookup Table) for 16-digit UIDs.
  * Reduces complexity by a factor of 4 billion ($2^{32}$).
  * **Empirical Whitelist**: Built-in filters derived from real-world data (2000+ samples) eliminate 99.9% of false positives.
* **🕰️ History Traversal**: Bypasses the "7-day limit" using the History API to trace old danmaku.
* **✅ Auto Verification**: Automatically verifies candidates against Bilibili's API to ensure 100% accuracy.

## 🛠️ Setup Guide

### 1. Requirements

* **Compiler**: GCC 8.0+ or Clang (MinGW-w64 on Windows).
* **Library**: `libcurl` (for HTTP requests).

### 2. Dependency Setup

Since this is a source-only distribution:

1. Download `libcurl` dev package.
2. Extract to `deps/` in the project root.
3. **Windows Users**: Copy `libcurl-x64.dll` to the same folder as `bilitrace.exe`.

## 📦 Build Instructions

```bash
# Windows (MinGW)
mingw32-make

# Linux
make
```

## 💻 Usage

```bash
./bilitrace.exe -bvid <BV_ID> -sessdata <COOKIE> -search "keywords" -first
```

## ❓ FAQ

* **Command not found?**: The executable is named `bilitrace.exe`. Do not look for `_v14` or other version numbers.
* **First run slow?**: It needs to generate the `mitm_table.bin` lookup table (800MB+). This is a one-time process.

## 🔬 Technical Principles (Math & Algo)

### 1. The Math of CRC32

Bilibili Danmaku ID is generated by: `DanmakuID = Hex(CRC32(UID))`.
Since CRC32 is a linear function over the Galois Field GF(2), it satisfies linearty:
$$CRC(A \oplus B) = CRC(A) \oplus CRC(B)$$

### 2. The "16-digit Trap"

* **Legacy UIDs (<10 digits)**: The search space is small ($10^{10} \approx 2^{33}$). A modern CPU can brute-force this in seconds.
* **Modern UIDs (16 digits)**: The search space is massive ($10^{16} \approx 2^{53}$). Brute-forcing this would take **300+ years** on a single core.

### 3. MITM Attack (Space-Time Tradeoff)

We split the 16-digit UID into two parts: `High` (first 6 digits) and `Low` (last 10 digits).
Using linearity:
$$CRC(High \times 10^{10}) \oplus CRC(Low) = TargetHash$$

We can rewrite the matching condition as:
$$CRC(High \times 10^{10}) = TargetHash \oplus CRC(Low)$$

* **Step 1 (Pre-computation)**: We compute the LHS for all valid `High` prefixes and store them in a **Lookup Table (Flash Map)**. This trades RAM (~2.4GB) for speed.
* **Step 2 (Online Search)**: We iterate through all possible `Low` values ($0-10^{10}$), compute the RHS, and check for existence in the table.

This reduces the complexity from $O(N)$ to roughly $O(\sqrt{N})$.

### 4. Empirical Optimization

We don't search blindly. By analyzing thousands of real user UIDs, we discovered that valid 16-digit UIDs are clustered. BiliTraceC uses a **Smart Whitelist** to only search prevalent prefixes (e.g., `35469...`), reducing the workload by 99% and enabling sub-minute cracking times.

## ⚠️ Disclaimer

For **Educational and Research Purposes Only**. Do not use for harassment or privacy violations. The authors assume no liability for misuse.

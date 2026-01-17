# BiliTraceC 进阶作业指南 (Standard Operating Procedure)

## 📌 核心理念：数据驱动的动态溯源

**BiliTraceC 不仅仅是一个静态程序，而是一个“侦察-打击”一体化的溯源系统。**

Bilibili 的 UID 分发策略（Snowflake 算法）是动态变化的。一个仅仅硬编码了旧规则的工具，面对新注册的用户（如 16 位 UID 的新号段）将无能为力。为了确保 **100% 的命中率** 和 **秒级破解速度**，所有用户都应当遵循本指南的 **“先侦察，后打击”** 标准作业流程。

---

## �️ 第一阶段：全栈环境部署 (Infrastructure)

要发挥本工具的全部性能，你需要一个完整的开发环境。

### 1.1 核心编译器 (GCC)

* **作用**: 将 C 源码编译为机器码，释放 CPU 的 SIMD 并行计算潜力。
* **安装**:
    1. 下载 [MinGW-w64 (x86_64-8.1.0)](https://github.com/niXman/mingw-builds-binaries/releases)。
    2. 解压并将 `bin` 文件夹添加到系统 `Path` 环境变量。
    3. 验证: `gcc --version`。

### 1.2 侦察工具 (Python 3)

* **作用**: 运行数据分析脚本，从 B站实时环境抓取最新的 UID 分布规律。
* **安装**:
    1. 下载并安装 [Python 3.x](https://www.python.org/)。
    2. 安装依赖库:

        ```bash
        pip install requests
        ```

### 1.3 网络军火库 (libcurl)

* **作用**: 只有配置了 libcurl，核心程序才能进行 HTTP 通信（历史回溯、API 验证）。
* **部署**:
    1. 下载 `curl-*-win64-mingw.zip`。
    2. 解压至项目根目录 `deps/`。
    3. **关键**: 将 `bin/libcurl-x64.dll` 复制到项目根目录。

---

## 🛰️ 第二阶段：战场侦察 (Reconnaissance)

在开始破解之前，我们需要获取当前 B站最新的 UID 分布情报。

### 2.1 运行前缀分析器

此脚本会扫描 B站当前热门视频的评论区，采集数千个活跃用户的 UID，并利用聚类算法分析出最新的 16 位 UID 前缀（如 `35469`）。

```bash
cd BiliTraceC
python scripts/analyze_uid_prefixes.py
```

### 2.2 获取情报

脚本运行结束后，会输出类似以下的统计信息，并在目录下生成 **`generated_whitelist.c`**：

```text
[Step 3] 生成 C 代码白名单 (5位前缀)
...
有效前缀数量: 14
包含: 34615, 34930, 35469 ...
[已保存] generated_whitelist.c
```

这个文件包含了针对当前 B站环境最优的过滤规则。

---

## ⚙️ 第三阶段：武器装填与铸造 (Weaponization)

将侦察到的情报注入核心程序，打造针对性的破解工具。

### 3.1 注入白名单

1. 打开生成的 `generated_whitelist.c`。
2. 打开源码 `src/mitm_cracker.c`。
3. 找到 `is_valid_16digit_prefix` 函数。
4. 将新生成的 `switch-case` 代码块 替换掉 `src/mitm_cracker.c` 中的旧代码。

### 3.2 编译铸造 (Build)

使用最新的情报重新编译程序。

```bash
# 清理旧构建
mingw32-make clean

# 编译新核心
mingw32-make
```

🚀 **现在，你拥有了一个针对当前 B站环境特化的 BiliTraceC 版本。**

---

## 🎯 第四阶段：精确打击 (Engagement)

使用编译好的神器进行溯源。

### 4.1 准备弹药 (SESSDATA)

即使拥有最強的算法，没有权限也无法查看历史弹幕。

* 在 B站按 `F12` -> `Application` -> `Cookies` 获取 `SESSDATA`。

### 4.2 发动攻击

使用标准命令进行回溯。建议始终开启 `-first` (单发命中) 模式以节省时间。

```bash
.\bilitrace.exe -bvid BV12P6UBLEdA -sessdata "YOUR_SESS" -search "目标弹幕" -first
```

### 4.3 战果确认

系统输出包含三个层级的信息：

1. **[Core]**: 暴力破解结果（针对老用户）。
2. **[MITM]**: 16位 UID 破解结果（高级引擎）。
    * *注意：首次运行会生成 800MB+ 查找表，请耐心等待 1-10 秒。*
3. **[验证]**: 最终 API 实测结果（✅存在 / ❌不存在）。

---

## 🔄 维护周期

**B站的 UID 生成规则通常每 3-6 个月变化一次。**
建议每隔一段时间（或当你发现命中率下降时），重新执行 **第二阶段 (Python 侦察)** 并重新编译。这将确保你的工具永远处于“版本答案”状态。

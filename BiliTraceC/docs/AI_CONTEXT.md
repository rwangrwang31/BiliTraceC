# BiliTraceC 项目上下文文档 (AI Context)

## 项目概述

**BiliTraceC** 是一个高性能的 Bilibili 弹幕发送者溯源工具。它通过逆向 CRC32 算法，将弹幕的匿名 Hash (8位16进制) 还原为真实的发送者 UID。

## 核心架构

本项目采用 **C 语言** 编写，注重正确性与稳定性。

### 1. 破解核心 (`cracker.c`)

- **策略**: **多线程并行穷举 (Multi-threaded Parallel Search)**
- **范围**: 0 - 5,000,000,000 (0-50亿)
- **并发安全**: 使用 `ThreadContext` 结构体存储线程本地结果，`atomic_int` 作为早停信号，主线程归约取最小 UID。
- **性能**: Intel Ultra 9 (24线程) 约 3 秒完成全空间扫描。
- **状态**: **完全可靠**。

### 2. 双模式支持

- **实时模式**: 无需登录，直接抓取视频 XML 弹幕文件。
- **历史模式**: 需 `SESSDATA` Cookie，通过 Protobuf API 抓取历史弹幕（支持按日期回溯）。

### 3. 数据处理 (`main.c` / `proto_parser.c`)

- **Protobuf 规范化**: B站 Protobuf 接口返回的 Hash 会省略前导零（如 `08abc...` 存为 `8abc...`）。
  - **处理**: `main.c` 在爆破前会自动检测并左补零至 8 位，确保输入正确。

## 关键业务逻辑 (Edge Cases)

### 1. 账号注销/封禁

- **现象**: 破解出的 UID 在 B站查询返回 404 或“用户不存在”。
- **结论**: **这是正常的**。
- **判定逻辑**: 只要 Hash 匹配 (CRC32(UID) == Hash)，该 UID 就是唯一的发送者。404 仅代表账号当前状态，不影响溯源结果的真实性。

### 2. Hash 碰撞

- **现状**: CRC32 存在碰撞，同一 Hash 可能对应多个 UID。
- **策略**: 工具返回**最小的碰撞 UID**（通过移除早停信号，让所有线程找完各自范围的第一个匹配，再归约取最小值）。
- **辅助工具**: `find_collisions.exe <hash>` 可列出所有碰撞候选。

### 3. 自动回退机制

- **行为**: 如果历史模式搜索完毕未找到匹配，工具自动切换到实时 XML 模式。
- **原因**: 新弹幕可能还未进入历史归档。

## 维护指南

- **修改搜索上限**: 编辑 `cracker.c` 中的 `max_uid`。
- **添加新字段**: 编辑 `proto_parser.c`，参照 B站 `dm.proto` 定义。
- **编译**: 使用 `gcc` (Linux/MinGW) 或 `cl` (MSVC)。推荐使用 `-O3` 优化。

## 当前文件清单

- `main.c`: 主入口，业务逻辑
- `cracker.c`: 单线程核心算法
- `proto_parser.c`: 手写 Protobuf 解析
- `history_api.c`: 历史 API 交互
- `network.c`: HTTP 请求封装

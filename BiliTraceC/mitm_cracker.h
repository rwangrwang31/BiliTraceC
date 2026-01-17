#ifndef MITM_CRACKER_H
#define MITM_CRACKER_H

#include <stdint.h>

// ============== 配置常量 ==============
#define LOW_PART_LIMIT 100000000 // 10^8 (低8位数字范围)
#define LOW_PART_DIGITS 8        // 低位数字位数
#define MAX_MITM_RESULTS 2000000 // 最大碰撞候选数 (扩容至200万，约16MB内存)

// ============== 数据结构 ==============

// CRC32 表项
typedef struct {
  uint32_t crc; // CRC32 值
  uint32_t low; // 对应的低位数值
} CrcEntry;

// MITM 结果集
typedef struct {
  uint64_t *uids; // 动态分配的 UID 数组
  int count;      // 当前找到的数量
  int capacity;   // 数组容量
} MitmResult;

// ============== 函数声明 ==============

/**
 * 初始化 MITM 模块 (加载或构建表)
 *
 * @param cache_path 缓存文件路径 (NULL 使用默认路径 "mitm_table.bin")
 * @return 0=成功, -1=失败
 *
 * 说明:
 *   - 如果缓存文件存在，直接加载 (<100ms)
 *   - 如果不存在，执行预计算并保存 (~0.8s with 24 threads)
 */
int mitm_init(const char *cache_path);

/**
 * 使用 MITM 攻击破解 CRC32 哈希
 *
 * @param target_hash 目标 CRC32 哈希值 (十六进制字符串，如 "90a567c7")
 * @param result 输出参数，存储所有匹配的 UID
 * @return 找到的候选数量，或 -1 表示错误
 *
 * 说明:
 *   - 支持 1-16 位数字 UID
 *   - 0.2 秒内完成全空间搜索
 *   - 可能返回多个碰撞候选
 */
int mitm_crack(const char *target_hash, MitmResult *result);

/**
 * 释放 MITM 模块资源
 */
void mitm_cleanup(void);

/**
 * 检查 MITM 模块是否已初始化
 * @return 1=已初始化, 0=未初始化
 */
int mitm_is_ready(void);

/**
 * 获取 MITM 表大小 (MB)
 */
size_t mitm_get_table_size_mb(void);

/**
 * 测试 MITM 数学逻辑
 * 用于验证 CRC32 combine 的"异或三明治"问题是否修复
 */
void test_mitm_logic(void);

#endif // MITM_CRACKER_H

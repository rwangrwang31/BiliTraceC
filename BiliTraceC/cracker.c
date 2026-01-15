/**
 * cracker.c
 * CRC32破解模块实现 - 单线程版本
 *
 * 采用简洁可靠的单线程设计，消除所有并发竞态问题
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "cracker.h"
#include "crc32_core.h"
#include "utils.h"

/**
 * 破解入口函数
 */
uint64_t crack_hash(const char *hex_hash, int thread_count) {
  (void)thread_count; // 忽略线程数参数，保持API兼容性

  // 将16进制字符串转换为整数便于比较
  uint32_t target = (uint32_t)strtoul(hex_hash, NULL, 16);

  printf("[Core] 启动单线程爆破 Hash: %08x\n", target);

  char buf[16];
  size_t len;

  // B站UID目前约在45亿以内，搜索至50亿保险
  uint64_t max_uid = 5000000000ULL;

  for (uint64_t uid = 0; uid < max_uid; uid++) {
    // 进度显示（每1亿次）
    if (uid % 100000000 == 0 && uid > 0) {
      printf("[Progress] %I64u / %I64u\r", uid, max_uid);
      fflush(stdout);
    }

    // 将UID转换为字符串
    len = fast_uid_to_str(uid, buf);

    // 计算CRC32并匹配
    if (crc32_fast(buf, len) == target) {
      printf("\n[Found] UID: %I64u\n", uid);
      return uid;
    }
  }

  printf("\n[NotFound] 在 0-%I64u 范围内未找到匹配\n", max_uid);
  return 0;
}

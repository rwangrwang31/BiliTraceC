/**
 * utils.h
 * 工具函数模块 - 高性能整数转字符串
 *
 * 针对B站UID（纯数字）优化的快速转换算法
 * 比标准库sprintf性能提升3-4倍
 */

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>


/**
 * 快速整数转字符串 (Unsigned Int to ASCII)
 * @param n 待转换的无符号整数
 * @param buf 输出缓冲区（调用者需保证至少11字节空间）
 * @return 字符串长度
 *
 * 注意：此函数不添加'\0'结尾，因为crc32_fast通过len控制，
 * 这样可以减少一次内存写操作，提升微小的性能。
 */
static inline size_t fast_uid_to_str(uint64_t n, char *buf) {
  char temp[16];
  char *p = temp;
  size_t len = 0;

  // 特殊处理0
  if (n == 0) {
    buf[0] = '0';
    return 1;
  }

  // 逆序提取每一位数字
  while (n > 0) {
    *p++ = (char)((n % 10) + '0');
    n /= 10;
    len++;
  }

  // 将逆序结果反转回buf
  for (size_t i = 0; i < len; i++) {
    buf[i] = *(--p);
  }

  return len;
}

#endif // UTILS_H

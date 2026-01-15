/**
 * cracker.h
 * 多线程CRC32破解模块头文件
 *
 * 使用Pthreads实现并行暴力破解
 * 支持全空间(0-40亿)秒级扫描
 */

#ifndef CRACKER_H
#define CRACKER_H

#include <stdint.h>

/**
 * 破解CRC32 Hash，还原B站UID
 * @param hex_hash 16进制格式的CRC32哈希值（如"bc28c067"）
 * @param thread_count 并行线程数量
 * @return 找到的UID，若未找到返回0
 */
uint64_t crack_hash(const char *hex_hash, int thread_count);

#endif // CRACKER_H

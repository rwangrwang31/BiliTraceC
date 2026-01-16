/**
 * cracker.c
 * CRC32破解模块实现 - 多线程版本 (Intel Core Ultra 9 优化)
 *
 * 设计原则：
 * 1. 线程本地化结果：每个线程仅写入自己的上下文，无共享写入
 * 2. 原子早停信号：使用 stdatomic 通知所有线程停止
 * 3. 主线程归约：join 后取最小命中 UID
 */

#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "cracker.h"
#include "crc32_core.h"
#include "utils.h"

// ============== 配置常量 ==============
#define MAX_THREADS 64
#define DEFAULT_THREADS 24    // Intel Ultra 9: 8P + 16E = 24
#define MAX_UID 5000000000ULL // 50亿

// ============== 线程上下文结构 ==============
typedef struct {
  uint64_t start_uid;   // 搜索起始UID
  uint64_t end_uid;     // 搜索结束UID (不包含)
  uint32_t target_hash; // 目标CRC32值
  uint64_t result_uid;  // 线程本地结果，0表示未找到
  int found;            // 线程本地标志
  int thread_id;        // 线程编号 (用于调试)
} ThreadContext;

// ============== 全局原子停止信号 ==============
static atomic_int g_stop_signal = 0;

// ============== Windows 线程兼容层 ==============
#ifdef _WIN32
typedef HANDLE thread_t;
typedef DWORD(WINAPI *thread_func_t)(void *);
#define THREAD_CREATE(t, func, arg)                                            \
  (*(t) =                                                                      \
       CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(func), (arg), 0, NULL),  \
   *(t) != NULL ? 0 : -1)
#define THREAD_JOIN(t) WaitForSingleObject(t, INFINITE)
#else
typedef pthread_t thread_t;
typedef void *(*thread_func_t)(void *);
#define THREAD_CREATE(t, func, arg) pthread_create(t, NULL, func, arg)
#define THREAD_JOIN(t) pthread_join(t, NULL)
#endif

// ============== 工作线程函数 ==============
#ifdef _WIN32
static DWORD WINAPI worker_thread(void *arg) {
#else
static void *worker_thread(void *arg) {
#endif
  ThreadContext *ctx = (ThreadContext *)arg;
  char buf[16];
  size_t len;

  for (uint64_t uid = ctx->start_uid; uid < ctx->end_uid; uid++) {
    // 不使用早停信号，确保每个线程都能找到自己范围内的第一个匹配
    // 这样才能保证归约时得到全局最小的 UID（处理 CRC32 碰撞）

    // 将UID转换为字符串
    len = fast_uid_to_str(uid, buf);

    // 计算CRC32并匹配
    if (crc32_fast(buf, len) == ctx->target_hash) {
      ctx->result_uid = uid;
      ctx->found = 1;
      // 找到第一个匹配后立即退出本线程（不通知其他线程）
#ifdef _WIN32
      return 0;
#else
      return NULL;
#endif
    }
  }

#ifdef _WIN32
  return 0;
#else
  return NULL;
#endif
}

// ============== 主入口函数 ==============
uint64_t crack_hash(const char *hex_hash, int thread_count) {
  // 参数校验与默认值
  if (thread_count <= 0 || thread_count > MAX_THREADS) {
    thread_count = DEFAULT_THREADS;
  }

  // 将16进制字符串转换为整数
  uint32_t target = (uint32_t)strtoul(hex_hash, NULL, 16);

  printf("[Core] 启动 %d 线程爆破 Hash: %08x (范围: 0-%I64u)\n", thread_count,
         target, MAX_UID);

  // 重置全局停止信号
  atomic_store(&g_stop_signal, 0);

  // 分配线程上下文数组
  ThreadContext contexts[MAX_THREADS];
  thread_t threads[MAX_THREADS];

  // 计算每个线程的搜索范围
  uint64_t chunk_size = MAX_UID / thread_count;
  uint64_t remainder = MAX_UID % thread_count;

  for (int i = 0; i < thread_count; i++) {
    contexts[i].start_uid = i * chunk_size;
    contexts[i].end_uid = (i + 1) * chunk_size;
    if (i == thread_count - 1) {
      contexts[i].end_uid += remainder; // 最后一个线程处理余数
    }
    contexts[i].target_hash = target;
    contexts[i].result_uid = 0;
    contexts[i].found = 0;
    contexts[i].thread_id = i;

    // 创建线程
    if (THREAD_CREATE(&threads[i], worker_thread, &contexts[i]) != 0) {
      fprintf(stderr, "[Error] 无法创建线程 %d\n", i);
      // 等待已创建的线程
      for (int j = 0; j < i; j++) {
        THREAD_JOIN(threads[j]);
      }
      return 0;
    }
  }

  // 进度显示线程 (可选，简化实现)
  printf("[Progress] 所有线程已启动，等待结果...\n");

  // 等待所有线程完成
  for (int i = 0; i < thread_count; i++) {
    THREAD_JOIN(threads[i]);
  }

  // 主线程归约：找最小命中UID
  uint64_t result = 0;
  for (int i = 0; i < thread_count; i++) {
    if (contexts[i].found) {
      if (result == 0 || contexts[i].result_uid < result) {
        result = contexts[i].result_uid;
      }
    }
  }

  if (result > 0) {
    printf("\n[Found] UID: %I64u (线程归约完成)\n", result);
  } else {
    printf("\n[NotFound] 在 0-%I64u 范围内未找到匹配\n", MAX_UID);
  }

  return result;
}

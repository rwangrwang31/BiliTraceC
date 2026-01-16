/**
 * find_all_collisions.c
 * 搜索所有匹配指定 CRC32 的 UID（不停止，找完整个空间）
 */
#include "crc32_core.h"
#include "utils.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s <hex_hash>\n", argv[0]);
    return 1;
  }

  uint32_t target = (uint32_t)strtoul(argv[1], NULL, 16);
  printf("Searching ALL matches for hash: %08x\n", target);
  printf("Range: 0 - 5,000,000,000\n");
  printf("==================================================\n");

  char buf[16];
  size_t len;
  int match_count = 0;
  uint64_t matches[100]; // 假设最多100个碰撞

  for (uint64_t uid = 0; uid < 5000000000ULL; uid++) {
    if (uid % 100000000 == 0 && uid > 0) {
      printf("Progress: %I64u / 5,000,000,000\r", uid);
      fflush(stdout);
    }

    len = fast_uid_to_str(uid, buf);
    if (crc32_fast(buf, len) == target) {
      matches[match_count++] = uid;
      printf("\n[MATCH #%d] UID: %I64u\n", match_count, uid);
    }
  }

  printf("\n\n==================================================\n");
  printf("Total matches found: %d\n", match_count);
  for (int i = 0; i < match_count; i++) {
    printf("  %d. UID %I64u -> https://space.bilibili.com/%I64u\n", i + 1,
           matches[i], matches[i]);
  }

  return 0;
}

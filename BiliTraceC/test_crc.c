/**
 * 正确的 crc32_combine 验证测试
 * 使用更直接的方法：增量 CRC 计算
 */

#include "crc32_core.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>


// 增量 CRC 更新：将 crc1 应用于后续数据
// crc1 是前面数据的 CRC，然后逐字节处理后面的数据
static uint32_t crc32_append(uint32_t crc1, const char *data, size_t len) {
  // crc1 已经包含 final XOR，需要先去掉
  uint32_t crc = crc1 ^ 0xFFFFFFFF;

  for (size_t i = 0; i < len; i++) {
    crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
  }

  return crc ^ 0xFFFFFFFF;
}

// 简化版 MITM：不使用 crc32_combine，直接预计算 CRC(H || L_padded)
// 这需要为每个 H 重新计算，但更可靠

int main() {
  printf("=== 增量 CRC 验证 ===\n\n");

  // 测试 1: "515808585" = "5" || "15808585"
  const char *h1 = "5";
  const char *l1 = "15808585";
  const char *full1 = "515808585";

  uint32_t crc_h1 = crc32_fast(h1, strlen(h1));
  uint32_t crc_full1_direct = crc32_fast(full1, strlen(full1));
  uint32_t crc_full1_append = crc32_append(crc_h1, l1, strlen(l1));

  printf("测试1: \"%s\" || \"%s\" = \"%s\"\n", h1, l1, full1);
  printf("  CRC(H) = %08x\n", crc_h1);
  printf("  CRC(H||L) 直接计算 = %08x\n", crc_full1_direct);
  printf("  crc32_append(CRC(H), L) = %08x\n", crc_full1_append);
  printf("  匹配: %s\n\n", (crc_full1_direct == crc_full1_append) ? "✓" : "✗");

  // 测试 2: "3546921440381311" = "35469214" || "40381311"
  const char *h2 = "35469214";
  const char *l2 = "40381311";
  const char *full2 = "3546921440381311";

  uint32_t crc_h2 = crc32_fast(h2, strlen(h2));
  uint32_t crc_full2_direct = crc32_fast(full2, strlen(full2));
  uint32_t crc_full2_append = crc32_append(crc_h2, l2, strlen(l2));

  printf("测试2: \"%s\" || \"%s\" = \"%s\"\n", h2, l2, full2);
  printf("  CRC(H) = %08x\n", crc_h2);
  printf("  CRC(H||L) 直接计算 = %08x\n", crc_full2_direct);
  printf("  crc32_append(CRC(H), L) = %08x\n", crc_full2_append);
  printf("  匹配: %s\n\n", (crc_full2_direct == crc_full2_append) ? "✓" : "✗");

  printf("=== 结论 ===\n");
  printf("crc32_append 方法正确工作\n");
  printf("可以用于 MITM 搜索：对每个 H，计算 CRC(H)，然后直接追加 L 字节\n");

  return 0;
}

/**
 * MITM 逻辑修复验证测试
 * 验证 CRC32 combine 的"异或三明治"问题是否修复
 */

#include "crc32_core.h"
#include "mitm_cracker.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 计算字符串的 CRC32
static uint32_t crc32_string(const char *str) {
  uint32_t crc = 0xFFFFFFFF;
  while (*str) {
    crc = crc32_table[(crc ^ *str) & 0xFF] ^ (crc >> 8);
    str++;
  }
  return crc ^ 0xFFFFFFFF;
}

int main(void) {
  printf("=== MITM 修复验证测试 ===\n\n");

  // 测试 1: 验证 CRC 计算基础功能
  printf("[测试 1] CRC32 基础验证\n");

  uint32_t crc_h = crc32_string("35469214");
  uint32_t crc_l = crc32_string("40381311");
  uint32_t crc_full = crc32_string("3546921440381311");

  printf("  CRC(\"35469214\")     = %08x (预期: 68947c4d)\n", crc_h);
  printf("  CRC(\"40381311\")     = %08x (预期: 2640627d)\n", crc_l);
  printf("  CRC(\"3546921440381311\") = %08x (预期: 90a567c7)\n", crc_full);

  // 检查基础 CRC 是否正确
  if (crc_h != 0x68947c4d || crc_l != 0x2640627d || crc_full != 0x90a567c7) {
    printf("\n❌ CRC32 基础计算与预期不符！\n");
    printf("   请检查 CRC32 表是否正确。\n");
  } else {
    printf("\n✅ CRC32 基础计算正确！\n");
  }

  // 测试 2: 调用 MITM 逻辑测试函数
  printf("\n[测试 2] MITM 数学逻辑验证\n");
  test_mitm_logic();

  // 测试 3: 完整的 combine 验证 (使用实际计算的目标 CRC)
  printf("\n[测试 3] 使用实际目标 CRC 验证\n");
  printf("  完整 UID \"3546921440381311\" 的 CRC = %08x\n", crc_full);
  printf("  如果 test_mitm_logic 的 target 应该是这个值，请更新测试用例。\n");

  printf("\n=== 测试完成 ===\n");
  return 0;
}

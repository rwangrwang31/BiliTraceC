/**
 * zlib 行为深入检查
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>


int main(void) {
  printf("=== zlib Behavior Check ===\n\n");

  // 基础测试
  const char *s1 = "test";
  const char *s2 = "40381311";
  const char *s3 = "3546921440381311";

  uLong crc1 = crc32(crc32(0L, Z_NULL, 0), (const Bytef *)s1, strlen(s1));
  uLong crc2 = crc32(crc32(0L, Z_NULL, 0), (const Bytef *)s2, strlen(s2));
  uLong crc3 = crc32(crc32(0L, Z_NULL, 0), (const Bytef *)s3, strlen(s3));

  printf("zlib CRC(\"%s\") = 0x%08lx\n", s1, crc1);
  printf("zlib CRC(\"%s\") = 0x%08lx\n", s2, crc2);
  printf("zlib CRC(\"%s\") = 0x%08lx\n", s3, crc3);

  // 逐字节计算 s2
  printf("\n=== zlib step-by-step for \"%s\" ===\n", s2);
  uLong step_crc = crc32(0L, Z_NULL, 0);
  printf("After init: 0x%08lx\n", step_crc);
  for (int i = 0; i < (int)strlen(s2); i++) {
    step_crc = crc32(step_crc, (const Bytef *)&s2[i], 1);
    printf("After '%c': 0x%08lx\n", s2[i], step_crc);
  }

  // 检查 combine
  printf("\n=== crc32_combine test ===\n");
  const char *h = "35469214";
  const char *l = "40381311";
  uLong crc_h = crc32(crc32(0L, Z_NULL, 0), (const Bytef *)h, strlen(h));
  uLong crc_l = crc32(crc32(0L, Z_NULL, 0), (const Bytef *)l, strlen(l));
  uLong combined = crc32_combine(crc_h, crc_l, strlen(l));

  printf("CRC(H) = 0x%08lx\n", crc_h);
  printf("CRC(L) = 0x%08lx\n", crc_l);
  printf("combine(H,L,8) = 0x%08lx\n", combined);
  printf("CRC(H||L) = 0x%08lx\n", crc3);
  printf("Match: %s\n", (combined == crc3) ? "YES" : "NO");

  // 关键: 检查 zlib 版本
  printf("\n=== zlib version ===\n");
  printf("ZLIB_VERSION: %s\n", ZLIB_VERSION);
  printf("zlibVersion(): %s\n", zlibVersion());

  return 0;
}

/**
 * main.c
 * BiliTraceC - B站弹幕溯源工具 v2.0 (History Edition)
 */

// Split blocks to prevent auto-sorting headers alphabetically
#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
#include <shellapi.h>
#define SLEEP_MS(x) Sleep(x)
#else
#include <unistd.h>
#define SLEEP_MS(x) usleep((x) * 1000)
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cracker.h"
#include "history_api.h"
#include "network.h"

// Helper to convert UTF-16 to UTF-8
char *wide_to_utf8(const wchar_t *wstr) {
  if (!wstr)
    return NULL;
  int size_needed =
      WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
  char *str = (char *)malloc(size_needed);
  WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, size_needed, NULL, NULL);
  return str;
}

// Helper to get command line args in UTF-8
void get_utf8_args(int *argc, char ***argv) {
  int wargc;
  wchar_t **wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
  if (!wargv)
    return;

  *argv = (char **)malloc(sizeof(char *) * (wargc + 1));
  *argc = wargc;

  for (int i = 0; i < wargc; i++) {
    (*argv)[i] = wide_to_utf8(wargv[i]);
  }
  (*argv)[wargc] = NULL;
  LocalFree(wargv);
}

// 默认配置
#define DEFAULT_THREADS 8
#define DEFAULT_LIMIT 20
#define SEARCH_LIMIT 100000

// 搜索上下文
typedef struct {
  const char *keyword;
  int threads;
  int total_processed;
  int total_matched;
} SearchContext;

/**
 * 打印程序横幅
 */
void print_banner(void) {
  printf("\n");
  printf("╔══════════════════════════════════════════════════════════╗\n");
  printf("║     BiliTraceC - B站弹幕溯源工具 v2.1 (History)          ║\n");
  printf("║     基于CRC32逆向工程的高性能C语言实现                   ║\n");
  printf("╚══════════════════════════════════════════════════════════╝\n");
  printf("\n");
}

/**
 * 打印使用帮助
 */
void usage(const char *prog) {
  printf("用法:\n");
  printf("  在线抓取 (实时):      %s -cid <CID> [-search <关键词>]\n", prog);
  printf("  历史回溯 (鉴权):      %s -cid <CID> -sessdata <COOKIE> [-search "
         "<关键词>]\n",
         prog);
  printf("  直接解密 (离线):      %s -hash <CRC32_HASH>\n", prog);
  printf("\n");
  printf("示例:\n");
  printf("  %s -cid 35268920394 -search \"ENTP\"\n", prog);
  printf("  %s -cid 35268920394 -sessdata \"xxx\" -search \"丢失的弹幕\"\n",
         prog);
  printf("\n");
}

// 处理单个历史弹幕的回调
int history_callback(DanmakuElem *elem, void *user_data) {
  SearchContext *ctx = (SearchContext *)user_data;
  ctx->total_processed++;

  // 检查关键词
  int match = 0;
  if (ctx->keyword) {
    if (elem->content && strstr(elem->content, ctx->keyword)) {
      match = 1;
    }
  } else {
    match = 1; // 无关键词则全部匹配（慎用，输出太多）
  }

  if (match) {
    ctx->total_matched++;
    printf("┌─────────────────────────────────────────────────────────\n");
    // Use %I64d for MinGW compatibility
    printf("│ [历史] 弹幕 #%d (日期: %I64d)\n", ctx->total_matched,
           elem->ctime);
    printf("├─────────────────────────────────────────────────────────\n");
    printf("│ 内容: %s\n", elem->content ? elem->content : "[NULL]");

    if (elem->midHash) {
      size_t len = strlen(elem->midHash);
      printf("│ Hash: [%s] (Len: %zu)\n", elem->midHash, len);

      // Validation Check
      int valid_hex = 1;
      if (len != 8)
        valid_hex = 0;
      for (size_t i = 0; i < len; i++) {
        char c = elem->midHash[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F'))) {
          valid_hex = 0;
          break;
        }
      }

      if (!valid_hex) {
        printf("│ [警告] Hash 格式异常！(Len: %zu)\n", len);
        printf("│ Raw Bytes: ");
        for (size_t i = 0; i < len; i++)
          printf("%02x ", (unsigned char)elem->midHash[i]);
        printf("\n");
      }

      // 关键修复：规范化 Hash 到 8 位（左补零）
      // Protobuf 存储时会丢失前导零，如 "87c8c3d" 应为 "087c8c3d"
      char normalized_hash[9] = {0};
      if (len < 8) {
        // 左补零
        size_t zeros_needed = 8 - len;
        for (size_t i = 0; i < zeros_needed; i++)
          normalized_hash[i] = '0';
        memcpy(normalized_hash + zeros_needed, elem->midHash, len);
        printf("│ [规范化] %s -> %s\n", elem->midHash, normalized_hash);
      } else {
        memcpy(normalized_hash, elem->midHash, 8);
      }

      // 使用规范化后的 Hash 爆破
      uint64_t uid =
          crack_hash(normalized_hash, elem->midHash ? ctx->threads : 0);
      if (uid) {
        printf("│ UID : %I64u\n", uid);
        printf("│ 主页: https://space.bilibili.com/%I64u\n", uid);
      } else {
        printf("│ UID : 未找到\n");
      }
    } else {
      printf("│ Hash: [无]\n");
    }
    printf("└─────────────────────────────────────────────────────────\n\n");
  }

  return 0; // Continue
}

/**
 * 解析XML (实时模式，保留旧逻辑)
 */
void parse_xml_legacy(const char *xml, const char *keyword, int limit,
                      int threads) {
  char *cursor = (char *)xml;
  int count = 0;
  int match_count = 0;

  printf("[系统] 开始解析实时XML数据...\n");
  while ((cursor = strstr(cursor, "<d p=\"")) != NULL && count < limit) {
    char *p_end = strchr(cursor + 6, '>');
    if (!p_end)
      break;
    char *c_end = strstr(p_end, "</d>");
    if (!c_end)
      break;

    size_t len = c_end - (p_end + 1);
    char *content = (char *)malloc(len + 1);
    if (content) {
      memcpy(content, p_end + 1, len);
      content[len] = 0;

      // Extract Hash (Simple version)
      char hash[16] = {0};
      char *p = cursor + 6;
      int commas = 0;
      while (p < p_end) {
        if (*p == ',') {
          commas++;
          if (commas == 6) {
            char *he = strchr(p + 1, ',');
            if (!he || he > p_end)
              he = strchr(p + 1, '"');
            if (!he || he > p_end)
              he = p_end; // fallback
            if (he && he - (p + 1) < 15) {
              strncpy(hash, p + 1, he - (p + 1));
            }
            break;
          }
        }
        p++;
      }

      int should_print = 0;
      if (!keyword || strstr(content, keyword))
        should_print = 1;

      if (should_print && strlen(hash) > 0) {
        match_count++;
        printf("[实时] %s (Hash: %s) -> ", content, hash);
        uint64_t uid = crack_hash(hash, threads);
        if (uid)
          printf("UID: %I64u\n", uid);
        else
          printf("UID: ???\n");
      }
      free(content);
    }
    cursor = c_end + 4;
    count++;
  }
  printf("[系统] 实时扫描结束。\n");
}

int main(int argc, char *argv[]) {
#ifdef _WIN32
  get_utf8_args(&argc, &argv);
  SetConsoleOutputCP(CP_UTF8);
#endif

  print_banner();

  if (argc < 2) {
    usage(argv[0]);
    return 0;
  }

  char *hash_target = NULL;
  long long cid = 0;
  char *search_keyword = NULL;
  char *sessdata = NULL;
  int limit = DEFAULT_LIMIT;
  int threads = DEFAULT_THREADS;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-hash") == 0 && i + 1 < argc)
      hash_target = argv[++i];
    else if (strcmp(argv[i], "-cid") == 0 && i + 1 < argc)
      cid = atoll(argv[++i]);
    else if (strcmp(argv[i], "-search") == 0 && i + 1 < argc)
      search_keyword = argv[++i];
    else if (strcmp(argv[i], "-sessdata") == 0 && i + 1 < argc)
      sessdata = argv[++i];
    else if (strcmp(argv[i], "-threads") == 0 && i + 1 < argc)
      threads = atoi(argv[++i]);
  }

  if (hash_target) {
    uint32_t uid = crack_hash(hash_target, threads);
    printf("Result: %u\n", uid);
    return 0;
  }

  if (cid > 0) {
    network_init();

    if (sessdata) {
      // === History Mode ===
      printf("[模式] 历史回溯 (鉴权模式)\n");
      printf("[警告] 请确保 SESSDATA 属于测试账号，高频访问有封号风险！\n\n");

      HistoryIndex *idx = fetch_history_index(cid, sessdata);
      if (idx) {
        SearchContext ctx = {search_keyword, threads, 0, 0};

        for (int i = 0; i < idx->count; i++) {
          if (idx->dates[i]) {
            fetch_history_segment(cid, idx->dates[i], sessdata,
                                  history_callback, &ctx);
            SLEEP_MS(2000); // 强制风控休眠 2秒
          }
        }
        free_history_index(idx);
      } else {
        printf("[错误] 无法获取历史索引，SESSDATA 可能无效或已过期。\n");
      }

    } else {
      // === Real-time Mode ===
      printf("[模式] 实时抓取 (匿名)\n");
      char *xml = fetch_danmaku(cid);
      if (xml) {
        parse_xml_legacy(xml, search_keyword, limit, threads);
        free(xml);
      }
    }

    network_cleanup();
  }

  return 0;
}

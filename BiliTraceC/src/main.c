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
#include <time.h>

#include "cracker.h"
#include "history_api.h"
#include "mitm_cracker.h"
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
#define MAX_SEEN_IDS 1000
typedef struct {
  const char *keyword;
  int threads;
  int total_processed;
  int total_matched;
  int first_only;                   // -first 模式：找到第一个就停
  int found;                        // 标记是否已找到（用于提前退出）
  long long seen_ids[MAX_SEEN_IDS]; // 简易去重：已见过的弹幕ID
  int seen_count;
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

  // 如果已经找到且是 first_only 模式，直接跳过
  if (ctx->first_only && ctx->found) {
    return 1; // 返回 1 表示停止遍历
  }

  ctx->total_processed++;

  // 去重检查：检查该弹幕ID是否已处理过
  for (int i = 0; i < ctx->seen_count; i++) {
    if (ctx->seen_ids[i] == elem->id) {
      return 0; // 已见过，跳过
    }
  }
  // 记录该ID
  if (ctx->seen_count < MAX_SEEN_IDS) {
    ctx->seen_ids[ctx->seen_count++] = elem->id;
  }

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
    ctx->found = 1; // 标记找到

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

      // 使用规范化后的 Hash 进行全量碰撞扫描
      CrackResult candidates;
      int count = crack_hash_all(normalized_hash, ctx->threads, &candidates);
      int found_valid_uid = 0; // 标记是否找到真实存在的 UID

      if (count > 0) {
        printf("│ 碰撞候选 (%d 个):\n", count);
        for (int i = 0; i < count; i++) {
          uint64_t uid = candidates.uids[i];
          int exists = verify_uid_exists(uid);
          // 只要有一个存在或未知(可能是网络问题)，我们就认为是有效尝试
          if (exists == 1)
            found_valid_uid = 1;

          const char *status = (exists == 1)   ? "✅存在"
                               : (exists == 0) ? "❌不存在"
                                               : "⚠️未知";
          printf("│   %d. UID %I64u (%s)\n", i + 1, uid, status);
          printf("│      主页: https://space.bilibili.com/%I64u\n", uid);
          // 请求间隔，避免风控
          if (i < count - 1) {
#ifdef _WIN32
            Sleep(500);
#else
            usleep(500000);
#endif
          }
        }
      } else {
        printf("│ UID : 未找到 (范围 0-%llu)\n",
               (unsigned long long)8000000000000ULL);
      }

      // =========================================================
      // MITM 自动回退逻辑: 如果暴力破解失败或全是无效碰撞
      // =========================================================
      if (!found_valid_uid) {
        printf("│\n");
        printf("│ [智能分析] 暴力破解未找到有效结果 (可能是16位长UID)\n");
        printf("│ [Core] 正在启动 MITM 攻击引擎 (全空间搜索)...\n");

        if (!mitm_is_ready()) {
          if (mitm_init(NULL) != 0) {
            printf("│ [Error] MITM 引擎初始化失败！\n");
            goto after_mitm;
          }
        }

        // 结构体本身很小，直接放在栈上
        // uids 数组由 mitm_crack 内部动态分配
        MitmResult mitm_candidates = {0};

        mitm_crack(normalized_hash, &mitm_candidates);

        if (mitm_candidates.count > 0 && mitm_candidates.uids != NULL) {
          printf("│\n");
          printf("│ MITM 候选 (%d 个) - 开始 API 验证:\n",
                 mitm_candidates.count);

          int verified_count = 0;
          for (int i = 0; i < mitm_candidates.count; i++) {
            uint64_t uid = mitm_candidates.uids[i];
            int exists = verify_uid_exists(uid);

            if (exists == 1) {
              // 找到有效 UID
              printf("│   %d. UID %I64u (✅存在)\n", i + 1, uid);
              printf("│      主页: https://space.bilibili.com/%I64u\n", uid);
              found_valid_uid = 1;
              verified_count++;

              if (ctx->first_only) {
                printf("│ [系统] 已找到有效目标，停止验证剩余候选。\n");
                break;
              }
            } else {
              // 每 100 个输出一次进度
              if ((i + 1) % 100 == 0) {
                printf("│   [进度] 已验证 %d/%d (暂无命中)\n", i + 1,
                       mitm_candidates.count);
              }
            }

            // 请求间隔 (150ms)
            if (i < mitm_candidates.count - 1) {
#ifdef _WIN32
              Sleep(150);
#else
              usleep(150000);
#endif
            }
          }

          if (!found_valid_uid) {
            printf("│ [完成] 验证 %d 个候选，未找到有效 UID\n",
                   mitm_candidates.count);
          }
        } else {
          printf("│ [MITM] 智能过滤后未找到匹配 UID (请检查规则)\n");
        }

        // 释放动态分配的内存
        if (mitm_candidates.uids) {
          free(mitm_candidates.uids);
          mitm_candidates.uids = NULL;
        }
      }

    after_mitm:;

    } else {
      printf("│ Hash: [无]\n");
    }
    printf("└─────────────────────────────────────────────────────────\n\n");

    // 如果是 first_only 模式，找到后立即返回停止信号
    if (ctx->first_only) {
      printf("[系统] 已找到目标弹幕，停止搜索。\n");
      return 1; // 停止
    }
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
  char *bvid_str = NULL;
  char *search_keyword = NULL;
  char *sessdata = NULL;
  int limit = DEFAULT_LIMIT;
  int threads = DEFAULT_THREADS;
  int first_only = 0; // 默认全量模式，加 -first 启用单结果模式

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-hash") == 0 && i + 1 < argc)
      hash_target = argv[++i];
    else if (strcmp(argv[i], "-cid") == 0 && i + 1 < argc)
      cid = atoll(argv[++i]);
    else if (strcmp(argv[i], "-bvid") == 0 && i + 1 < argc)
      bvid_str = argv[++i];
    else if (strcmp(argv[i], "-search") == 0 && i + 1 < argc)
      search_keyword = argv[++i];
    else if (strcmp(argv[i], "-sessdata") == 0 && i + 1 < argc)
      sessdata = argv[++i];
    else if (strcmp(argv[i], "-threads") == 0 && i + 1 < argc)
      threads = atoi(argv[++i]);
    else if (strcmp(argv[i], "-first") == 0)
      first_only = 1;
  }

  if (hash_target) {
    // 使用 MITM 攻击支持 16 位 UID
    printf("[MITM] 初始化中间相遇攻击模块...\n");
    if (mitm_init(NULL) != 0) {
      fprintf(stderr, "[Error] MITM 初始化失败\n");
      return 1;
    }

    MitmResult result;
    int count = mitm_crack(hash_target, &result);

    if (count > 0) {
      printf("\n[结果] 找到 %d 个匹配 UID:\n", count);
      for (int i = 0; i < count; i++) {
        printf("  %d. UID %I64u\n", i + 1, result.uids[i]);
        printf("     主页: https://space.bilibili.com/%I64u\n", result.uids[i]);
      }
    } else {
      printf("[结果] 未找到匹配 UID\n");
    }

    mitm_cleanup();
    return 0;
  }

  // 如果提供了 BVID，自动获取 CID 和发布日期
  long long pub_ts = 0;
  if (bvid_str) {
    network_init();
    VideoInfo vinfo = {0};
    if (fetch_video_info(bvid_str, &vinfo)) {
      cid = vinfo.cid;
      pub_ts = vinfo.pubdate;
    } else {
      printf("[错误] 无法从 BVID 获取视频信息，请检查 BV 号是否正确。\n");
      network_cleanup();
      return 1;
    }
  }

  if (cid > 0) {
    if (!bvid_str) {
      network_init(); // 如果没有 BVID，这里才初始化网络
    }

    if (sessdata) {
      // === History Mode ===
      printf("[模式] 历史回溯 (鉴权模式)\n");
      printf("[原理] 正向遍历日期索引，突破7天限制\n");
      printf("[警告] 请确保 SESSDATA 属于测试账号，高频访问有封号风险！\n\n");

      // Start from current month
      char current_month[16] = "2026-01";
      char end_month[16] = "2009-01"; // Bilibili founded around 2009

      // 使用已获取的 pub_ts 来设置 end_month
      if (pub_ts > 0) {
#ifdef _WIN32
        struct tm *tm_info = localtime((time_t *)&pub_ts);
#else
        time_t pts = (time_t)pub_ts;
        struct tm *tm_info = localtime(&pts);
#endif
        if (tm_info) {
          snprintf(end_month, sizeof(end_month), "%04d-%02d",
                   tm_info->tm_year + 1900, tm_info->tm_mon + 1);
          printf("[系统] 回溯终点: %s (视频发布日期)\n", end_month);
        }
      }

      int empty_months_streak = 0;
      int history_found_any = 0; // 跟踪历史模式是否找到结果

      while (1) {
        if (strcmp(current_month, end_month) < 0) {
          printf("[系统] 已到达视频发布日期 (%s)，回溯结束。\n", end_month);
          break;
        }

        HistoryIndex *idx =
            fetch_history_index_by_month(cid, current_month, sessdata);

        if (!idx || idx->count == 0) {
          printf("[系统] %s 无数据\n", current_month);
          if (idx)
            free_history_index(idx);

          // Heuristic: stop if we see 6 consecutive empty months (faster abort)
          empty_months_streak++;
          if (empty_months_streak > 6) { // 6个月空档 -> 停止
            printf("[系统] 连续6个月无数据，停止回溯。\n");
            break;
          }
        } else {
          empty_months_streak = 0; // Reset streak
          SearchContext ctx = {0};
          ctx.keyword = search_keyword;
          ctx.threads = threads;
          ctx.first_only = first_only;
          ctx.found = 0;
          ctx.seen_count = 0;

          for (int i = 0; i < idx->count; i++) {
            if (idx->dates[i]) {
              fetch_history_segment(cid, idx->dates[i], sessdata,
                                    history_callback, &ctx);
              // 如果已找到且是 first_only 模式，立即退出
              if (ctx.first_only && ctx.found) {
                history_found_any = 1;
                free_history_index(idx);
                goto crawl_done; // 跳出多层循环
              }
              // 记录是否找到任何结果
              if (ctx.found)
                history_found_any = 1;
              // Dynamic sleep: 1.5s is safer
              SLEEP_MS(1500);
            }
          }
          free_history_index(idx);
        }

        // Decrement Month
        int y, m;
        sscanf(current_month, "%d-%d", &y, &m);
        m--;
        if (m == 0) {
          m = 12;
          y--;
        }
        snprintf(current_month, sizeof(current_month), "%04d-%02d", y, m);
      }

    crawl_done: // Label for early exit via goto
      // 如果历史模式没找到，且有关键词，自动尝试实时模式
      if (!history_found_any && search_keyword) {
        printf("\n[系统] 历史模式未找到匹配，自动切换到实时模式...\n\n");
        printf("[模式] 实时抓取 (匿名)\n");
        char *xml = fetch_danmaku(cid);
        if (xml) {
          parse_xml_legacy(xml, search_keyword, limit, threads);
          free(xml);
        }
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

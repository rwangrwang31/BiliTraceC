/**
 * bilitrace_local.c
 * BiliTraceC 本地版本 - 无网络依赖
 * 
 * 功能：
 * 1. 离线直接解密CRC32 Hash
 * 2. 从本地XML文件解析弹幕
 * 3. 支持关键词搜索
 * 
 * 编译命令：
 * gcc -O3 -Wall -o bilitrace.exe bilitrace_local.c -lpthread
 * 或 Windows原生线程：
 * gcc -O3 -Wall -DUSE_WIN_THREADS -o bilitrace.exe bilitrace_local.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

// ============================================================
// CRC32 核心算法 (内联)
// ============================================================

static const uint32_t crc32_table[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdede86c5, 0x47d7777f, 0x30d041e9,
    0xbddc0d18, 0xcadb3d8e, 0x53d26a34, 0x24d55aa2, 0xbab02001, 0xcdb71097, 0x54de512d, 0x23d967bb,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static inline uint32_t crc32_fast(const char *str, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        uint8_t table_index = (crc ^ (uint8_t)str[i]) & 0xFF;
        crc = (crc >> 8) ^ crc32_table[table_index];
    }
    return ~crc;
}

// ============================================================
// 快速整数转字符串
// ============================================================

static inline size_t fast_uid_to_str(uint32_t n, char *buf) {
    char temp[16];
    char *p = temp;
    size_t len = 0;

    if (n == 0) {
        buf[0] = '0';
        return 1;
    }

    while (n > 0) {
        *p++ = (char)((n % 10) + '0');
        n /= 10;
        len++;
    }

    for (size_t i = 0; i < len; i++) {
        buf[i] = *(--p);
    }

    return len;
}

// ============================================================
// 多线程支持
// ============================================================

#ifdef USE_WIN_THREADS
// Windows原生线程
#include <windows.h>
#define DEFAULT_THREADS 4

static volatile LONG g_found = 0;
static volatile uint32_t g_result_uid = 0;

typedef struct {
    uint32_t start;
    uint32_t end;
    uint32_t target;
    int id;
} thread_ctx_t;

DWORD WINAPI worker_thread(LPVOID arg) {
    thread_ctx_t *ctx = (thread_ctx_t *)arg;
    char buf[16];
    size_t len;
    uint32_t current_hash;

    for (uint32_t uid = ctx->start; uid < ctx->end; uid++) {
        if (g_found) return 0;
        
        len = fast_uid_to_str(uid, buf);
        current_hash = crc32_fast(buf, len);

        if (current_hash == ctx->target) {
            InterlockedExchange(&g_found, 1);
            g_result_uid = uid;
            return 0;
        }
    }
    return 0;
}

uint32_t crack_hash(const char *hex_hash, int thread_count) {
    uint32_t target = (uint32_t)strtoul(hex_hash, NULL, 16);
    
    HANDLE *threads = (HANDLE *)malloc(sizeof(HANDLE) * thread_count);
    thread_ctx_t *args = (thread_ctx_t *)malloc(sizeof(thread_ctx_t) * thread_count);

    if (!threads || !args) {
        fprintf(stderr, "[Error] Memory allocation failed\n");
        if (threads) free(threads);
        if (args) free(args);
        return 0;
    }

    g_found = 0;
    g_result_uid = 0;

    uint32_t max_uid = 4000000000U;
    uint32_t step = max_uid / thread_count;

    printf("[Core] Starting %d threads to crack Hash: %08x\n", thread_count, target);

    for (int i = 0; i < thread_count; i++) {
        args[i].start = (uint32_t)i * step;
        args[i].end = (i == thread_count - 1) ? max_uid : (uint32_t)(i + 1) * step;
        args[i].target = target;
        args[i].id = i;

        threads[i] = CreateThread(NULL, 0, worker_thread, &args[i], 0, NULL);
        if (threads[i] == NULL) {
            fprintf(stderr, "[Error] Thread %d creation failed\n", i);
        }
    }

    WaitForMultipleObjects(thread_count, threads, TRUE, INFINITE);

    for (int i = 0; i < thread_count; i++) {
        if (threads[i]) CloseHandle(threads[i]);
    }

    free(threads);
    free(args);
    return g_result_uid;
}

#else
// POSIX线程 (pthread)
#include <pthread.h>
#define DEFAULT_THREADS 8

static volatile int g_found = 0;
static volatile uint32_t g_result_uid = 0;

typedef struct {
    uint32_t start;
    uint32_t end;
    uint32_t target;
    int id;
} thread_ctx_t;

static void* worker_thread(void *arg) {
    thread_ctx_t *ctx = (thread_ctx_t *)arg;
    char buf[16];
    size_t len;
    uint32_t current_hash;

    for (uint32_t uid = ctx->start; uid < ctx->end; uid++) {
        if (g_found) return NULL;

        len = fast_uid_to_str(uid, buf);
        current_hash = crc32_fast(buf, len);

        if (current_hash == ctx->target) {
            g_found = 1;
            g_result_uid = uid;
            return NULL;
        }
    }
    return NULL;
}

uint32_t crack_hash(const char *hex_hash, int thread_count) {
    uint32_t target = (uint32_t)strtoul(hex_hash, NULL, 16);

    pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    thread_ctx_t *args = (thread_ctx_t *)malloc(sizeof(thread_ctx_t) * thread_count);

    if (!threads || !args) {
        fprintf(stderr, "[Error] Memory allocation failed\n");
        if (threads) free(threads);
        if (args) free(args);
        return 0;
    }

    g_found = 0;
    g_result_uid = 0;

    uint32_t max_uid = 4000000000U;
    uint32_t step = max_uid / thread_count;

    printf("[Core] Starting %d threads to crack Hash: %08x\n", thread_count, target);

    for (int i = 0; i < thread_count; i++) {
        args[i].start = (uint32_t)i * step;
        args[i].end = (i == thread_count - 1) ? max_uid : (uint32_t)(i + 1) * step;
        args[i].target = target;
        args[i].id = i;

        if (pthread_create(&threads[i], NULL, worker_thread, &args[i]) != 0) {
            fprintf(stderr, "[Error] Thread %d creation failed\n", i);
        }
    }

    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(args);
    return g_result_uid;
}
#endif

// ============================================================
// XML文件解析
// ============================================================

char* read_file(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "[Error] Cannot open file: %s\n", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *content = (char *)malloc(size + 1);
    if (!content) {
        fclose(fp);
        return NULL;
    }

    fread(content, 1, size, fp);
    content[size] = '\0';
    fclose(fp);

    return content;
}

void parse_and_crack(const char *xml, const char *keyword, int limit, int threads) {
    char *cursor = (char *)xml;
    int count = 0;
    int match_count = 0;

    if (keyword) {
        printf("[Mode] Searching for danmaku containing \"%s\"\n", keyword);
    }
    printf("[System] Starting to parse danmaku data...\n\n");

    while ((cursor = strstr(cursor, "<d p=\"")) != NULL && count < limit) {
        char *p_attr_start = cursor + 6;
        char *p_attr_end = strchr(p_attr_start, '>');

        if (!p_attr_end) break;

        char *content_start = p_attr_end + 1;
        char *content_end = strstr(content_start, "</d>");

        if (!content_end) break;

        // Extract danmaku text
        size_t text_len = content_end - content_start;
        char *text_buf = (char *)malloc(text_len + 1);
        if (!text_buf) break;
        memcpy(text_buf, content_start, text_len);
        text_buf[text_len] = '\0';

        // Extract Hash (7th field, index 6)
        char hash_buf[16] = {0};
        char *param_ptr = p_attr_start;
        int commas = 0;
        
        while (param_ptr < p_attr_end) {
            if (*param_ptr == ',') {
                commas++;
                if (commas == 6) {
                    char *hash_start = param_ptr + 1;
                    char *hash_end = strchr(hash_start, ',');
                    
                    if (hash_end && hash_end < p_attr_end) {
                        size_t len = hash_end - hash_start;
                        if (len < 15) strncpy(hash_buf, hash_start, len);
                    }
                    break;
                }
            }
            param_ptr++;
        }

        int should_process = 1;
        if (keyword) {
            if (!strstr(text_buf, keyword)) {
                should_process = 0;
            }
        }

        if (should_process && strlen(hash_buf) > 0) {
            match_count++;
            printf("+----------------------------------------------------------\n");
            printf("| Danmaku #%d\n", match_count);
            printf("+----------------------------------------------------------\n");
            printf("| Content: %s\n", text_buf);
            printf("| Hash: %s\n", hash_buf);

            uint32_t uid = crack_hash(hash_buf, threads);
            if (uid) {
                printf("| UID : %u\n", uid);
                printf("| URL : https://space.bilibili.com/%u\n", uid);
            } else {
                printf("| UID : Not found\n");
            }
            printf("+----------------------------------------------------------\n\n");
        }

        free(text_buf);
        cursor = content_end + 4;
        count++;
    }

    printf("==============================================================\n");
    printf(" Scan complete: processed %d danmaku", count);
    if (keyword) {
        printf(", matched %d", match_count);
    }
    printf("\n");
    printf("==============================================================\n");
}

// ============================================================
// 主程序
// ============================================================

void print_banner(void) {
    printf("\n");
    printf("==============================================================\n");
    printf("     BiliTraceC - Bilibili Danmaku Tracer v2.0 (Local)        \n");
    printf("     CRC32 Reverse Engineering Tool                           \n");
    printf("==============================================================\n");
    printf("\n");
}

void usage(const char *prog) {
    printf("Usage:\n");
    printf("  Crack Hash:        %s -hash <CRC32_HASH>\n", prog);
    printf("  Parse XML file:    %s -file <XML_FILE>\n", prog);
    printf("  Search in XML:     %s -file <XML_FILE> -search <KEYWORD>\n", prog);
    printf("  Set thread count:  %s -hash <HASH> -threads <N>\n", prog);
    printf("  Set limit:         %s -file <XML_FILE> -limit <N>\n", prog);
    printf("\n");
    printf("Examples:\n");
    printf("  %s -hash bc28c067\n", prog);
    printf("  %s -file danmaku.xml\n", prog);
    printf("  %s -file danmaku.xml -search \"666\" -limit 100\n", prog);
    printf("\n");
    printf("Note:\n");
    printf("  Download XML: curl -o danmaku.xml https://comment.bilibili.com/<CID>.xml\n");
    printf("\n");
}

int main(int argc, char *argv[]) {
    print_banner();

    if (argc < 2) {
        usage(argv[0]);
        return 0;
    }

    char *hash_target = NULL;
    char *xml_file = NULL;
    char *search_keyword = NULL;
    int limit = 20;
    int threads = DEFAULT_THREADS;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-hash") == 0 && i + 1 < argc) {
            hash_target = argv[++i];
        } else if (strcmp(argv[i], "-file") == 0 && i + 1 < argc) {
            xml_file = argv[++i];
        } else if (strcmp(argv[i], "-search") == 0 && i + 1 < argc) {
            search_keyword = argv[++i];
            limit = 100000;
        } else if (strcmp(argv[i], "-limit") == 0 && i + 1 < argc) {
            limit = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-threads") == 0 && i + 1 < argc) {
            threads = atoi(argv[++i]);
            if (threads < 1) threads = 1;
            if (threads > 64) threads = 64;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        }
    }

    if (hash_target) {
        printf("[Mode] Offline Hash Cracking\n");
        printf("[Target] Hash: %s\n", hash_target);
        printf("[Config] Threads: %d\n\n", threads);

        clock_t start = clock();
        uint32_t uid = crack_hash(hash_target, threads);
        clock_t end = clock();
        double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

        printf("\n");
        if (uid != 0) {
            printf("+==========================================================+\n");
            printf("|  [SUCCESS] Cracking Complete                             |\n");
            printf("+==========================================================+\n");
            printf("|  Hash: %-48s  |\n", hash_target);
            printf("|  UID : %-48u  |\n", uid);
            printf("|  URL : https://space.bilibili.com/%-20u  |\n", uid);
            printf("|  Time: %.3f seconds                                      |\n", elapsed);
            printf("+==========================================================+\n");
        } else {
            printf("+==========================================================+\n");
            printf("|  [FAILED] UID not found                                  |\n");
            printf("|  Possible: UID out of range (0-4 billion) or invalid hash|\n");
            printf("+==========================================================+\n");
        }
    }
    else if (xml_file) {
        printf("[Mode] Local XML Parsing\n");
        printf("[File] %s\n", xml_file);
        printf("[Config] Threads: %d, Limit: %d\n", threads, limit);
        if (search_keyword) {
            printf("[Search] Keyword: \"%s\"\n", search_keyword);
        }
        printf("\n");

        char *xml = read_file(xml_file);
        if (!xml) {
            return 1;
        }

        printf("[System] File loaded (%lu bytes)\n\n", (unsigned long)strlen(xml));
        parse_and_crack(xml, search_keyword, limit, threads);
        free(xml);
    }
    else {
        printf("[Error] Missing required parameters\n\n");
        usage(argv[0]);
        return 1;
    }

    return 0;
}

/**
 * network.c
 * 网络模块实现
 *
 * 使用libcurl从B站API获取弹幕XML数据
 * 自动处理HTTP压缩和内存管理
 */

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "network.h"

// 动态内存结构，用于存储下载的数据
struct MemoryStruct {
  char *memory;
  size_t size;
};

/**
 * libcurl的回调函数，处理接收到的数据块
 */
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb,
                                  void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  // 重新分配内存以容纳新数据
  char *ptr = (char *)realloc(mem->memory, mem->size + realsize + 1);
  if (!ptr) {
    fprintf(stderr, "[错误] 内存分配失败！\n");
    return 0;
  }

  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0; // 保持字符串以null结尾

  return realsize;
}

/**
 * 初始化网络模块
 */
void network_init(void) { curl_global_init(CURL_GLOBAL_ALL); }

/**
 * 清理网络模块
 */
void network_cleanup(void) { curl_global_cleanup(); }

/**
 * 根据CID下载弹幕XML
 */
char *fetch_danmaku(long long cid) {
  CURL *curl_handle;
  CURLcode res;
  struct MemoryStruct chunk;

  // 初始化内存结构
  chunk.memory = (char *)malloc(1);
  if (!chunk.memory) {
    fprintf(stderr, "[错误] 初始内存分配失败\n");
    return NULL;
  }
  chunk.size = 0;

  // 构建URL
  char url[256];
  // Fix format for MinGW
  snprintf(url, sizeof(url), "https://comment.bilibili.com/%I64d.xml", cid);

  curl_handle = curl_easy_init();
  if (curl_handle) {
    // 设置URL
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    // 设置回调函数和数据指针
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

    // 伪装User-Agent，防止被拦截
    curl_easy_setopt(
        curl_handle, CURLOPT_USERAGENT,
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36 BiliTraceC/2.0");

    // 自动处理gzip/deflate压缩
    curl_easy_setopt(curl_handle, CURLOPT_ACCEPT_ENCODING, "");

    // 设置超时（30秒）
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 30L);

    // 跟随重定向
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);

    // SSL验证配置
    // 尝试使用本地证书文件
    const char *ca_bundle = getenv("CURL_CA_BUNDLE");
    if (ca_bundle) {
      curl_easy_setopt(curl_handle, CURLOPT_CAINFO, ca_bundle);
    } else {
      // 尝试使用同目录下的证书文件
      curl_easy_setopt(curl_handle, CURLOPT_CAINFO, "curl-ca-bundle.crt");
    }

// 如果证书验证失败，可以暂时禁用（仅用于测试）
#ifdef DISABLE_SSL_VERIFY
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
#else
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 2L);
#endif

    // 执行请求
    res = curl_easy_perform(curl_handle);

    if (res != CURLE_OK) {
      fprintf(stderr, "[错误] 网络请求失败: %s\n", curl_easy_strerror(res));
      free(chunk.memory);
      curl_easy_cleanup(curl_handle);
      return NULL;
    }

    // 检查HTTP响应码
    long response_code;
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
    if (response_code != 200) {
      fprintf(stderr, "[错误] HTTP响应码: %ld\n", response_code);
      free(chunk.memory);
      curl_easy_cleanup(curl_handle);
      return NULL;
    }

    curl_easy_cleanup(curl_handle);
  } else {
    fprintf(stderr, "[错误] CURL初始化失败\n");
    free(chunk.memory);
    return NULL;
  }

  return chunk.memory;
}

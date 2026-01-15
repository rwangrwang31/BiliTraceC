#include "history_api.h"
#include "cJSON.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// 简单的内存缓冲区
struct MemoryStruct {
  char *memory;
  size_t size;
};

// libcurl 回调
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb,
                                  void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  char *ptr = (char *)realloc(mem->memory, mem->size + realsize + 1);
  if (!ptr) {
    printf("[Error] Out of memory!\n");
    return 0;
  }

  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

// 辅助：执行HTTP请求
static struct MemoryStruct perform_request(const char *url,
                                           const char *sessdata) {
  struct MemoryStruct chunk = {NULL, 0};
  chunk.memory = (char *)malloc(1);
  chunk.size = 0;

  CURL *curl = curl_easy_init();
  if (curl) {
    // Headers
    struct curl_slist *headers = NULL;
    // 伪装 User-Agent
    headers = curl_slist_append(headers,
                                "User-Agent: Mozilla/5.0 (Windows NT 10.0; "
                                "Win64; x64) AppleWebKit/537.36 (KHTML, like "
                                "Gecko) Chrome/120.0.0.0 Safari/537.36");
    // 设置 Cookie
    if (sessdata) {
      char cookie_buf[512];
      snprintf(cookie_buf, sizeof(cookie_buf), "Cookie: SESSDATA=%s", sessdata);
      headers = curl_slist_append(headers, cookie_buf);
    }

    // 设置URL
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, ""); // Handle gzip

    // SSL (Disable verify for simplicity in this env, but warn user in real
    // code)
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      fprintf(stderr, "[Error] curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
      free(chunk.memory);
      chunk.memory = NULL;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
  }
  return chunk;
}

void history_init(void) {
  // curl_global_init is handled in main network module usually, but safe to
  // call
}

HistoryIndex *fetch_history_index(long long cid, const char *sessdata) {
  char url[256];
  // Use %I64d for MinGW compatibility
  snprintf(url, sizeof(url),
           "https://api.bilibili.com/x/v2/dm/history/"
           "index?type=1&oid=%I64d&month=2026-01",
           cid);

  struct MemoryStruct response = perform_request(url, sessdata);
  if (!response.memory)
    return NULL;

  // Parse JSON
  cJSON *json = cJSON_Parse(response.memory);
  if (!json) {
    printf("[Error] Failed to parse JSON index\n");
    free(response.memory);
    return NULL;
  }

  // Check code
  cJSON *code = cJSON_GetObjectItem(json, "code");
  cJSON *data = cJSON_GetObjectItem(json, "data");

  if (!cJSON_IsNumber(code) || code->valueint != 0 || !cJSON_IsArray(data)) {
    printf("[Error] API returned error or no data: %s\n", response.memory);
    cJSON_Delete(json);
    free(response.memory);
    return NULL;
  }

  // Extract dates
  int count = cJSON_GetArraySize(data);
  HistoryIndex *idx = (HistoryIndex *)malloc(sizeof(HistoryIndex));
  idx->count = count;
  idx->dates = (char **)malloc(sizeof(char *) * count);

  for (int i = 0; i < count; i++) {
    cJSON *item = cJSON_GetArrayItem(data, i);
    if (cJSON_IsString(item)) {
      idx->dates[i] = strdup(item->valuestring);
    } else {
      idx->dates[i] = NULL;
    }
  }

  printf("[System] Found %d dates in history index (2026-01)\n", count);

  cJSON_Delete(json);
  free(response.memory);
  return idx;
}

void free_history_index(HistoryIndex *idx) {
  if (!idx)
    return;
  for (int i = 0; i < idx->count; i++) {
    if (idx->dates[i])
      free(idx->dates[i]);
  }
  free(idx->dates);
  free(idx);
}

int fetch_history_segment(long long cid, const char *date, const char *sessdata,
                          DanmakuCallback cb, void *user_data) {
  char url[256];
  snprintf(url, sizeof(url),
           "https://api.bilibili.com/x/v2/dm/web/history/"
           "seg.so?type=1&oid=%I64d&date=%s",
           cid, date);

  printf("[Network] Downloading history segment for %s...\n", date);
  struct MemoryStruct response = perform_request(url, sessdata);

  if (!response.memory)
    return -1;

  if (response.size == 0) {
    printf("[Warn] Empty response for %s\n", date);
    free(response.memory);
    return 0;
  }

  // Parse Protobuf
  ProtoResult res =
      parse_dm_seg((uint8_t *)response.memory, response.size, cb, user_data);

  free(response.memory);

  if (res != PROTO_OK) {
    printf("[Error] Protobuf parse error: %d\n", res);
    return -1;
  }

  return 0; // Success
}

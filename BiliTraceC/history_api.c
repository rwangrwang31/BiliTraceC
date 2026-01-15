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

HistoryIndex *fetch_history_index_by_month(long long cid, const char *month,
                                           const char *sessdata) {
  char url[256];
  // Use %I64d for MinGW compatibility
  snprintf(url, sizeof(url),
           "https://api.bilibili.com/x/v2/dm/history/"
           "index?type=1&oid=%I64d&month=%s",
           cid, month);

  struct MemoryStruct response = perform_request(url, sessdata);
  if (!response.memory)
    return NULL;

  // Parse JSON
  cJSON *json = cJSON_Parse(response.memory);
  if (!json) {
    printf("[Error] Failed to parse JSON index for %s\n", month);
    free(response.memory);
    return NULL;
  }

  // Check code
  cJSON *code = cJSON_GetObjectItem(json, "code");
  cJSON *data = cJSON_GetObjectItem(json, "data");

  // Allow "data": null which means empty month (not error)
  if (!cJSON_IsNumber(code) || code->valueint != 0) {
    printf("[Error] API returned error for %s: %s\n", month, response.memory);
    cJSON_Delete(json);
    free(response.memory);
    return NULL;
  }

  HistoryIndex *idx = (HistoryIndex *)malloc(sizeof(HistoryIndex));
  if (!cJSON_IsArray(data)) {
    // Empty data for this month
    idx->count = 0;
    idx->dates = NULL;
  } else {
    // Extract dates
    int count = cJSON_GetArraySize(data);
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
  }

  printf("[System] Found %d dates in history index (%s)\n", idx->count, month);

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

long long fetch_video_pubdate(const char *bvid) {
  char url[256];
  snprintf(url, sizeof(url),
           "https://api.bilibili.com/x/web-interface/view?bvid=%s", bvid);

  // Note: View API usually doesn't need SESSDATA for public videos,
  // but if we have it, passing NULL is fine for this helper usually,
  // or we could pass sessdata if needed. For now, public view.
  struct MemoryStruct response = perform_request(url, NULL);

  if (!response.memory)
    return 0;

  cJSON *json = cJSON_Parse(response.memory);
  if (!json) {
    printf("[Error] Failed to parse View API JSON\n");
    free(response.memory);
    return 0;
  }

  cJSON *data = cJSON_GetObjectItem(json, "data");
  if (!data) {
    cJSON_Delete(json);
    free(response.memory);
    return 0;
  }

  cJSON *pubdate = cJSON_GetObjectItem(data, "pubdate");
  long long ts = 0;
  if (cJSON_IsNumber(pubdate)) {
    ts = (long long)pubdate->valuedouble;
  }

  cJSON_Delete(json);
  free(response.memory);
  return ts;
}

int fetch_video_info(const char *bvid, VideoInfo *info) {
  if (!bvid || !info)
    return 0;

  char url[256];
  snprintf(url, sizeof(url),
           "https://api.bilibili.com/x/web-interface/view?bvid=%s", bvid);

  struct MemoryStruct response = perform_request(url, NULL);

  if (!response.memory)
    return 0;

  cJSON *json = cJSON_Parse(response.memory);
  if (!json) {
    printf("[Error] Failed to parse View API JSON for %s\n", bvid);
    free(response.memory);
    return 0;
  }

  cJSON *code_obj = cJSON_GetObjectItem(json, "code");
  if (!cJSON_IsNumber(code_obj) || code_obj->valueint != 0) {
    printf("[Error] View API returned error for %s\n", bvid);
    cJSON_Delete(json);
    free(response.memory);
    return 0;
  }

  cJSON *data = cJSON_GetObjectItem(json, "data");
  if (!data) {
    cJSON_Delete(json);
    free(response.memory);
    return 0;
  }

  // 提取 CID (第一个分P)
  cJSON *cid_obj = cJSON_GetObjectItem(data, "cid");
  if (cJSON_IsNumber(cid_obj)) {
    info->cid = (long long)cid_obj->valuedouble;
  } else {
    info->cid = 0;
  }

  // 提取发布时间
  cJSON *pubdate_obj = cJSON_GetObjectItem(data, "pubdate");
  if (cJSON_IsNumber(pubdate_obj)) {
    info->pubdate = (long long)pubdate_obj->valuedouble;
  } else {
    info->pubdate = 0;
  }

  // 提取标题（可选）
  cJSON *title_obj = cJSON_GetObjectItem(data, "title");
  if (cJSON_IsString(title_obj) && title_obj->valuestring) {
    strncpy(info->title, title_obj->valuestring, sizeof(info->title) - 1);
    info->title[sizeof(info->title) - 1] = '\0';
  } else {
    info->title[0] = '\0';
  }

  cJSON_Delete(json);
  free(response.memory);

  printf("[系统] 获取视频信息成功: CID=%I64d, 发布于 %I64d\n", info->cid,
         info->pubdate);
  if (info->title[0]) {
    printf("[系统] 标题: %s\n", info->title);
  }

  return (info->cid > 0) ? 1 : 0;
}

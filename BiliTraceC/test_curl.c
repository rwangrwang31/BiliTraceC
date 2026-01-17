#include <curl/curl.h>
#include <stdio.h>


int main() {
  CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
  if (res == CURLE_OK) {
    printf("Curl init success\n");
    curl_global_cleanup();
  } else {
    printf("Curl init failed\n");
  }
  return 0;
}

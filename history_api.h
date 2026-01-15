#ifndef HISTORY_API_H
#define HISTORY_API_H

#include "proto_parser.h"

// 历史数据索引
typedef struct {
  int count;
  char **dates; // Array of date strings "YYYY-MM-DD"
} HistoryIndex;

// 初始化
void history_init(void);

// 获取历史弹幕日期索引
// 返回 HistoryIndex 结构，失败返回 NULL
// 获取历史弹幕日期索引 (指定月份)
// 返回 HistoryIndex 结构，失败返回 NULL
// month: "YYYY-MM"
// 必须在 headers 中携带 SESSDATA
HistoryIndex *fetch_history_index_by_month(long long cid, const char *month,
                                           const char *sessdata);

// 释放索引结构
void free_history_index(HistoryIndex *idx);

// 获取指定日期的历史数据并解析
// cid: 视频CID
// date: 日期字符串 "YYYY-MM-DD"
// sessdata: 用户凭证
// cb: 每一条弹幕的回调函数
// user_data: 用户数据
// 返回: 解析成功的弹幕数量，或 -1 表示失败
// 获取指定日期的历史数据并解析
// 返回: 解析成功的弹幕数量，或 -1 表示失败
int fetch_history_segment(long long cid, const char *date, const char *sessdata,
                          DanmakuCallback cb, void *user_data);

// 获取视频发布时间戳 (Pubdate)
// 需要提供 BVID (如 BV1xx...)
// Returns unix timestamp or 0 on failure
long long fetch_video_pubdate(const char *bvid);

#endif

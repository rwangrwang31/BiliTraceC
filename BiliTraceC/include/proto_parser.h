#ifndef PROTO_PARSER_H
#define PROTO_PARSER_H

#include <stddef.h>
#include <stdint.h>


// 状态码
typedef enum {
  PROTO_OK = 0,
  PROTO_ERR_INVALID_DATA = -1,
  PROTO_ERR_BUFFER_OVERFLOW = -2,
  PROTO_ERR_WIRE_TYPE_MISMATCH = -3,
  PROTO_ERR_VARINT_OVERFLOW = -4
} ProtoResult;

// 弹幕元素结构 (DanmakuElem)
typedef struct {
  int64_t id;       // field 1
  int32_t progress; // field 2
  int32_t mode;     // field 3
  int32_t fontsize; // field 4
  uint32_t color;   // field 5
  char *midHash;    // field 6 (Allocated)
  char *content;    // field 7 (Allocated)
  int64_t ctime;    // field 8
  int32_t weight;   // field 9
  char *action;     // field 10 (Allocated)
  int32_t pool;     // field 11
  char *idStr;      // field 12 (Allocated)
  int32_t attr;     // field 13
} DanmakuElem;

// 回调函数类型：每解析出一条弹幕调用一次
// 返回 0 表示继续，非 0 表示停止解析
typedef int (*DanmakuCallback)(DanmakuElem *elem, void *user_data);

// 解析历史数据分片
// data: Protobuf 二进制数据
// len: 数据长度
// callback: 处理每一条弹幕的回调函数
// user_data: 传递给回调的用户数据
ProtoResult parse_dm_seg(const uint8_t *data, size_t len,
                         DanmakuCallback callback, void *user_data);

// 释放弹幕结构体中的动态内存
void free_danmaku_elem(DanmakuElem *elem);

#endif // PROTO_PARSER_H

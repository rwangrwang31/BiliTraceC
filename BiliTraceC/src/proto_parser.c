#include "proto_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Protobuf Wire Types
 */
#define WT_VARINT 0
#define WT_64BIT 1
#define WT_LENGTH 2
#define WT_START 3
#define WT_END 4
#define WT_32BIT 5

/*
 * Helper: Read Varint
 */
static ProtoResult read_varint(const uint8_t **ptr, const uint8_t *end,
                               uint64_t *val) {
  const uint8_t *p = *ptr;
  uint64_t result = 0;
  int shift = 0;

  while (p < end && shift < 64) {
    uint8_t byte = *p;
    result |= ((uint64_t)(byte & 0x7F)) << shift;
    p++;
    if (!(byte & 0x80)) {
      *val = result;
      *ptr = p;
      return PROTO_OK;
    }
    shift += 7;
  }
  return (shift >= 64) ? PROTO_ERR_VARINT_OVERFLOW : PROTO_ERR_BUFFER_OVERFLOW;
}

/*
 * Helper: Skip Field based on Wire Type
 */
static ProtoResult skip_field(const uint8_t **ptr, const uint8_t *end,
                              int wire_type) {
  const uint8_t *p = *ptr;
  switch (wire_type) {
  case WT_VARINT: {
    uint64_t tmp;
    return read_varint(ptr, end, &tmp);
  }
  case WT_64BIT:
    if (p + 8 > end)
      return PROTO_ERR_BUFFER_OVERFLOW;
    *ptr += 8;
    return PROTO_OK;
  case WT_LENGTH: {
    uint64_t len;
    ProtoResult res = read_varint(ptr, end, &len);
    if (res != PROTO_OK)
      return res;
    if (*ptr + len > end)
      return PROTO_ERR_BUFFER_OVERFLOW;
    *ptr += len;
    return PROTO_OK;
  }
  case WT_32BIT:
    if (p + 4 > end)
      return PROTO_ERR_BUFFER_OVERFLOW;
    *ptr += 4;
    return PROTO_OK;
  default:
    return PROTO_ERR_WIRE_TYPE_MISMATCH;
  }
}

/*
 * Helper: Read String (Length Delimited)
 */
static ProtoResult read_string(const uint8_t **ptr, const uint8_t *end,
                               char **str_out) {
  uint64_t len;
  ProtoResult res = read_varint(ptr, end, &len);
  if (res != PROTO_OK)
    return res;
  if (*ptr + len > end)
    return PROTO_ERR_BUFFER_OVERFLOW;

  *str_out = (char *)malloc(len + 1);
  if (!*str_out)
    return PROTO_ERR_INVALID_DATA; // Alloc fail

  memcpy(*str_out, *ptr, len);
  (*str_out)[len] = '\0';
  *ptr += len;
  return PROTO_OK;
}

/*
 * Parse DanmakuElem (Nested Message)
 */
static ProtoResult parse_danmaku_elem(const uint8_t *data, size_t len,
                                      DanmakuElem *elem) {
  const uint8_t *ptr = data;
  const uint8_t *end = data + len;

  memset(elem, 0, sizeof(DanmakuElem));

  while (ptr < end) {
    uint64_t tag;
    ProtoResult res = read_varint(&ptr, end, &tag);
    if (res != PROTO_OK)
      return res;

    int field_num = tag >> 3;
    int wire_type = tag & 0x07;

    switch (field_num) {
    case 1: // id (int64)
      if (wire_type != WT_VARINT)
        return PROTO_ERR_WIRE_TYPE_MISMATCH;
      res = read_varint(&ptr, end, (uint64_t *)&elem->id);
      break;
    case 2: // progress (int32)
      if (wire_type != WT_VARINT)
        return PROTO_ERR_WIRE_TYPE_MISMATCH;
      {
        uint64_t v;
        res = read_varint(&ptr, end, &v);
        if (res != PROTO_OK)
          return res;
        elem->progress = (int32_t)v;
      }
      break;
    case 3: // mode
      if (wire_type != WT_VARINT)
        return PROTO_ERR_WIRE_TYPE_MISMATCH;
      {
        uint64_t v;
        res = read_varint(&ptr, end, &v);
        if (res != PROTO_OK)
          return res;
        elem->mode = (int32_t)v;
      }
      break;
    case 4: // fontsize
      if (wire_type != WT_VARINT)
        return PROTO_ERR_WIRE_TYPE_MISMATCH;
      {
        uint64_t v;
        res = read_varint(&ptr, end, &v);
        if (res != PROTO_OK)
          return res;
        elem->fontsize = (int32_t)v;
      }
      break;
    case 5: // color
      if (wire_type != WT_VARINT)
        return PROTO_ERR_WIRE_TYPE_MISMATCH;
      {
        uint64_t v;
        res = read_varint(&ptr, end, &v);
        if (res != PROTO_OK)
          return res;
        elem->color = (uint32_t)v;
      }
      break;
    case 6: // midHash (string)
      if (wire_type != WT_LENGTH)
        return PROTO_ERR_WIRE_TYPE_MISMATCH;
      res = read_string(&ptr, end, &elem->midHash);
      break;
    case 7: // content (string)
      if (wire_type != WT_LENGTH)
        return PROTO_ERR_WIRE_TYPE_MISMATCH;
      res = read_string(&ptr, end, &elem->content);
      break;
    case 8: // ctime
      if (wire_type != WT_VARINT)
        return PROTO_ERR_WIRE_TYPE_MISMATCH;
      res = read_varint(&ptr, end, (uint64_t *)&elem->ctime);
      break;
    case 9: // weight
      if (wire_type != WT_VARINT)
        return PROTO_ERR_WIRE_TYPE_MISMATCH;
      {
        uint64_t v;
        res = read_varint(&ptr, end, &v);
        if (res != PROTO_OK)
          return res;
        elem->weight = (int32_t)v;
      }
      break;
    case 10: // action
      if (wire_type != WT_LENGTH)
        return PROTO_ERR_WIRE_TYPE_MISMATCH;
      res = read_string(&ptr, end, &elem->action);
      break;
    case 11: // pool
      if (wire_type != WT_VARINT)
        return PROTO_ERR_WIRE_TYPE_MISMATCH;
      {
        uint64_t v;
        res = read_varint(&ptr, end, &v);
        if (res != PROTO_OK)
          return res;
        elem->pool = (int32_t)v;
      }
      break;
    case 12: // idStr
      if (wire_type != WT_LENGTH)
        return PROTO_ERR_WIRE_TYPE_MISMATCH;
      res = read_string(&ptr, end, &elem->idStr);
      break;
    default:
      // Skip unknown fields robustly to handle future protocol changes
      res = skip_field(&ptr, end, wire_type);
      break;
    }

    if (res != PROTO_OK) {
      free_danmaku_elem(elem);
      return res;
    }
  }
  return PROTO_OK;
}

/*
 * Parse DmSegMobileReply (Outer Message)
 * message DmSegMobileReply {
 *   repeated DanmakuElem elems = 1;
 *   int32 state = 2;
 * }
 */
ProtoResult parse_dm_seg(const uint8_t *data, size_t len,
                         DanmakuCallback callback, void *user_data) {
  const uint8_t *ptr = data;
  const uint8_t *end = data + len;

  while (ptr < end) {
    uint64_t tag;
    ProtoResult res = read_varint(&ptr, end, &tag);
    if (res != PROTO_OK)
      return res;

    int field_num = tag >> 3;
    int wire_type = tag & 0x07;

    if (field_num == 1) { // elems (repeated DanmakuElem)
      if (wire_type != WT_LENGTH)
        return PROTO_ERR_WIRE_TYPE_MISMATCH;

      uint64_t sub_len;
      res = read_varint(&ptr, end, &sub_len);
      if (res != PROTO_OK)
        return res;

      const uint8_t *sub_end = ptr + sub_len;
      if (sub_end > end)
        return PROTO_ERR_BUFFER_OVERFLOW;

      // Parse one DanmakuElem
      DanmakuElem elem;
      res = parse_danmaku_elem(ptr, sub_len, &elem);
      if (res == PROTO_OK) {
        // invoke callback
        if (callback) {
          if (callback(&elem, user_data) != 0) {
            free_danmaku_elem(&elem);
            return PROTO_OK; // Stop requested
          }
        }
        free_danmaku_elem(&elem);
      }
      // Move pointer after sub-message
      ptr += sub_len;

    } else if (field_num == 2) { // state
      if (wire_type != WT_VARINT)
        return PROTO_ERR_WIRE_TYPE_MISMATCH;
      uint64_t v;
      res = read_varint(&ptr, end, &v);
    } else {
      res = skip_field(&ptr, end, wire_type);
    }

    if (res != PROTO_OK)
      return res;
  }

  return PROTO_OK;
}

void free_danmaku_elem(DanmakuElem *elem) {
  if (elem->midHash)
    free(elem->midHash);
  if (elem->content)
    free(elem->content);
  if (elem->action)
    free(elem->action);
  if (elem->idStr)
    free(elem->idStr);
  memset(elem, 0, sizeof(DanmakuElem));
}

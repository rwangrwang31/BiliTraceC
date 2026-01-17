#!/usr/bin/env python3
"""
analyze_uid_prefixes.py - 16位 UID 前缀分析器 v2

流程：
1. 采集：爬取热门视频评论区，获取2000+活跃用户
2. 聚类：筛选16位UID，统计前缀分布 (5位粒度)
3. 输出：生成 C 代码白名单规则
"""

import time
from collections import Counter
from typing import List, Set, Tuple

import requests

# ============ 配置 ============
TARGET_COUNT = 2000  # 目标采集数量
REQUEST_DELAY = 0.4  # 请求间隔

HEADERS = {
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
                  "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36",
    "Referer": "https://www.bilibili.com/",
}


def fetch_hot_videos() -> List[str]:
    """获取热门视频列表"""
    url = "https://api.bilibili.com/x/web-interface/popular"
    try:
        resp = requests.get(url, params={"ps": 50, "pn": 1}, headers=HEADERS, timeout=10)
        data = resp.json()
        if data.get("code") == 0:
            return [v["bvid"] for v in data["data"]["list"]]
    except Exception as e:
        print(f"[Error] 获取热门视频失败: {e}")
    return []


def fetch_video_aid(bvid: str) -> int:
    """获取视频 AID"""
    url = "https://api.bilibili.com/x/web-interface/view"
    try:
        resp = requests.get(url, params={"bvid": bvid}, headers=HEADERS, timeout=10)
        data = resp.json()
        if data.get("code") == 0:
            return data["data"]["aid"]
    except:
        pass
    return 0


def fetch_comments(aid: int, page: int = 1) -> List[int]:
    """获取评论区用户"""
    url = "https://api.bilibili.com/x/v2/reply"
    try:
        resp = requests.get(url, params={"type": 1, "oid": aid, "pn": page, "ps": 20, "sort": 2},
                          headers=HEADERS, timeout=10)
        data = resp.json()
        if data.get("code") == 0 and data.get("data"):
            uids = []
            for r in data["data"].get("replies", []) or []:
                mid = r.get("member", {}).get("mid")
                if mid:
                    uids.append(int(mid))
                for sub in r.get("replies", []) or []:
                    sub_mid = sub.get("member", {}).get("mid")
                    if sub_mid:
                        uids.append(int(sub_mid))
            return uids
    except Exception as e:
        print(f"  [Error] 评论获取失败: {e}")
    return []


def is_16digit(uid: int) -> bool:
    """判断是否为16位UID"""
    return 1000000000000000 <= uid < 10000000000000000


def get_prefix(uid: int, length: int = 5) -> str:
    """获取UID前缀"""
    return str(uid)[:length]


def collect_uids() -> Set[int]:
    """采集用户UID"""
    print("=" * 60)
    print("Step 1: 采集活跃用户 UID")
    print("=" * 60)
    
    all_uids = set()
    
    # 获取热门视频列表
    print("\n[获取热门视频列表...]")
    hot_videos = fetch_hot_videos()
    print(f"  获取到 {len(hot_videos)} 个热门视频")
    
    if not hot_videos:
        print("[Error] 无法获取热门视频！")
        return all_uids
    
    # 从评论区采集
    for i, bvid in enumerate(hot_videos):
        if len(all_uids) >= TARGET_COUNT:
            break
        
        aid = fetch_video_aid(bvid)
        time.sleep(REQUEST_DELAY)
        
        if not aid:
            continue
        
        print(f"\n  [{i+1}/{len(hot_videos)}] {bvid} (aid={aid}):")
        
        for page in range(1, 11):  # 每个视频取10页
            uids = fetch_comments(aid, page)
            time.sleep(REQUEST_DELAY)
            
            if not uids:
                break
            
            all_uids.update(uids)
            
            # 统计16位数量
            uids_16 = [u for u in uids if is_16digit(u)]
            print(f"    第{page}页: +{len(uids)} (16位: {len(uids_16)}) 总计: {len(all_uids)}")
            
            if len(all_uids) >= TARGET_COUNT:
                break
    
    print(f"\n[采集完成] 共获取 {len(all_uids)} 个唯一 UID")
    return all_uids


def analyze_prefixes(uids: Set[int]) -> Tuple[Counter, Counter]:
    """分析16位UID的前缀分布"""
    print("\n" + "=" * 60)
    print("Step 2: 聚类分析 16位 UID 前缀 (5位粒度)")
    print("=" * 60)
    
    # 筛选16位UID
    uids_16 = [u for u in uids if is_16digit(u)]
    pct = 100*len(uids_16)/len(uids) if uids else 0
    print(f"\n16位UID数量: {len(uids_16)} / {len(uids)} ({pct:.1f}%)")
    
    if not uids_16:
        print("[警告] 未找到16位UID！")
        return Counter(), Counter()
    
    # 统计前5位前缀
    prefix5_counter = Counter(get_prefix(u, 5) for u in uids_16)
    
    # 输出前5位分布
    print("\n前5位前缀分布:")
    print("-" * 40)
    for prefix, count in prefix5_counter.most_common():
        bar = "█" * min(count, 30)
        print(f"  {prefix}: {count:3d} {bar}")
    
    return prefix5_counter, prefix5_counter


def generate_c_code(prefix_counter: Counter) -> str:
    """生成 C 代码白名单"""
    print("\n" + "=" * 60)
    print("Step 3: 生成 C 代码白名单 (5位前缀)")
    print("=" * 60)
    
    valid_prefixes = sorted([int(p) for p, c in prefix_counter.items()])
    
    code_lines = [
        "// 基于真实用户数据的 16位UID 前缀白名单 (5位粒度)",
        "// 自动生成，数据来源: B站热门视频评论区",
        "// is_likely_valid_uid 函数中使用",
        "",
        "// 有效前缀数量: " + str(len(valid_prefixes)),
        "static int is_valid_16digit_prefix(uint64_t uid) {",
        "  uint64_t prefix = uid / 100000000000ULL;  // 取前5位",
        "  switch (prefix) {",
    ]
    
    for prefix in valid_prefixes:
        code_lines.append(f"    case {prefix}:")
    
    code_lines.extend([
        "      return 1;",
        "    default:",
        "      return 0;",
        "  }",
        "}",
    ])
    
    code = "\n".join(code_lines)
    print("\n" + code)
    
    with open("generated_whitelist.c", "w") as f:
        f.write(code)
    print(f"\n[已保存] generated_whitelist.c ({len(valid_prefixes)} 个前缀)")
    
    return code


def main():
    print("\n" + "=" * 60)
    print("16位 UID 前缀分析器 v2")
    print("=" * 60)
    
    uids = collect_uids()
    
    if not uids:
        print("[Error] 采集失败！")
        return
    
    prefix4, prefix6 = analyze_prefixes(uids)
    
    if prefix4:
        generate_c_code(prefix4)
    
    print("\n[完成]")


if __name__ == "__main__":
    main()

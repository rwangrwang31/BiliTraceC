#!/usr/bin/env python3
"""
build_uid_whitelist.py - 16位 UID 白名单构建器

通过爬取大 UP 主的粉丝列表和热门视频评论区，
收集真实存在的 16 位 Bilibili UID，构建白名单数据库。

使用方法:
    python build_uid_whitelist.py --mode fans --uid 123456
    python build_uid_whitelist.py --mode comments --bvid BV1xxx
    python build_uid_whitelist.py --mode batch
"""

import argparse
import json
import os
import re
import sqlite3
import time
from typing import List, Optional, Set

import requests

# ============ 配置 ============
DB_FILE = "uid_whitelist.db"
REQUEST_DELAY = 0.5  # 请求间隔
MAX_PAGES = 50       # 每个来源最多爬取的页数

HEADERS = {
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
                  "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36",
    "Referer": "https://www.bilibili.com/",
}

# 热门 UP 主列表 (用于批量爬取)
POPULAR_UP_MIDS = [
    546195,     # 老番茄
    1850091,    # 罗翔说刑法
    7584632,    # 何同学
    927587,     # 敖厂长
    2206456,    # 蜡笔小新
]

# 热门视频列表 (用于批量爬取评论)
POPULAR_BVIDS = [
    "BV1GJ411x7h7",  # 示例热门视频
]


def init_db():
    """初始化 SQLite 数据库"""
    conn = sqlite3.connect(DB_FILE)
    cursor = conn.cursor()
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS uid_whitelist (
            uid INTEGER PRIMARY KEY,
            source TEXT,
            collected_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    ''')
    conn.commit()
    return conn


def save_uids(conn: sqlite3.Connection, uids: Set[int], source: str):
    """保存 UID 到数据库"""
    cursor = conn.cursor()
    saved = 0
    for uid in uids:
        try:
            cursor.execute(
                "INSERT OR IGNORE INTO uid_whitelist (uid, source) VALUES (?, ?)",
                (uid, source)
            )
            if cursor.rowcount > 0:
                saved += 1
        except Exception:
            pass
    conn.commit()
    return saved


def is_16digit_uid(uid: int) -> bool:
    """检查是否为 16 位 UID"""
    return 1000000000000000 <= uid < 10000000000000000


def fetch_followers(mid: int, page: int = 1) -> List[int]:
    """获取 UP 主的粉丝列表"""
    url = f"https://api.bilibili.com/x/relation/followers"
    params = {"vmid": mid, "pn": page, "ps": 50}
    
    try:
        resp = requests.get(url, params=params, headers=HEADERS, timeout=10)
        data = resp.json()
        if data.get("code") == 0 and data.get("data"):
            followers = data["data"].get("list", [])
            return [f["mid"] for f in followers if f.get("mid")]
    except Exception as e:
        print(f"  [Error] 获取粉丝列表失败: {e}")
    
    return []


def fetch_comments(bvid: str, page: int = 1) -> List[int]:
    """获取视频评论区的用户"""
    # 先获取 aid
    url = f"https://api.bilibili.com/x/web-interface/view"
    try:
        resp = requests.get(url, params={"bvid": bvid}, headers=HEADERS, timeout=10)
        data = resp.json()
        if data.get("code") != 0:
            return []
        aid = data["data"]["aid"]
    except Exception:
        return []
    
    # 获取评论
    url = f"https://api.bilibili.com/x/v2/reply"
    params = {"type": 1, "oid": aid, "pn": page, "ps": 20, "sort": 2}
    
    try:
        resp = requests.get(url, params=params, headers=HEADERS, timeout=10)
        data = resp.json()
        if data.get("code") == 0 and data.get("data"):
            replies = data["data"].get("replies", []) or []
            uids = []
            for r in replies:
                if r.get("member", {}).get("mid"):
                    uids.append(r["member"]["mid"])
                # 子评论
                for sub in r.get("replies", []) or []:
                    if sub.get("member", {}).get("mid"):
                        uids.append(sub["member"]["mid"])
            return uids
    except Exception as e:
        print(f"  [Error] 获取评论失败: {e}")
    
    return []


def collect_from_fans(conn: sqlite3.Connection, mid: int, max_pages: int = MAX_PAGES):
    """从粉丝列表收集 16 位 UID"""
    print(f"\n[Fans] 正在爬取 UP 主 {mid} 的粉丝列表...")
    all_uids = set()
    
    for page in range(1, max_pages + 1):
        uids = fetch_followers(mid, page)
        if not uids:
            print(f"  第 {page} 页: 无数据或到达末尾")
            break
        
        # 筛选 16 位 UID
        uids_16 = {u for u in uids if is_16digit_uid(u)}
        all_uids.update(uids_16)
        
        print(f"  第 {page} 页: 获取 {len(uids)} 个 UID, 其中 16 位: {len(uids_16)}")
        time.sleep(REQUEST_DELAY)
    
    # 保存
    saved = save_uids(conn, all_uids, f"fans:{mid}")
    print(f"  [完成] 收集 {len(all_uids)} 个 16 位 UID, 新增 {saved} 个")
    return all_uids


def collect_from_comments(conn: sqlite3.Connection, bvid: str, max_pages: int = MAX_PAGES):
    """从评论区收集 16 位 UID"""
    print(f"\n[Comments] 正在爬取视频 {bvid} 的评论区...")
    all_uids = set()
    
    for page in range(1, max_pages + 1):
        uids = fetch_comments(bvid, page)
        if not uids:
            print(f"  第 {page} 页: 无数据或到达末尾")
            break
        
        uids_16 = {u for u in uids if is_16digit_uid(u)}
        all_uids.update(uids_16)
        
        print(f"  第 {page} 页: 获取 {len(uids)} 个 UID, 其中 16 位: {len(uids_16)}")
        time.sleep(REQUEST_DELAY)
    
    saved = save_uids(conn, all_uids, f"comments:{bvid}")
    print(f"  [完成] 收集 {len(all_uids)} 个 16 位 UID, 新增 {saved} 个")
    return all_uids


def batch_collect(conn: sqlite3.Connection):
    """批量爬取预设的热门来源"""
    print("=" * 60)
    print("批量收集模式")
    print("=" * 60)
    
    total_collected = 0
    
    # 爬取热门 UP 主的粉丝
    for mid in POPULAR_UP_MIDS:
        uids = collect_from_fans(conn, mid, max_pages=20)
        total_collected += len(uids)
        time.sleep(2)  # 来源间隔
    
    # 爬取热门视频的评论
    for bvid in POPULAR_BVIDS:
        uids = collect_from_comments(conn, bvid, max_pages=20)
        total_collected += len(uids)
        time.sleep(2)
    
    print(f"\n[批量完成] 共收集 {total_collected} 个 16 位 UID")


def query_stats(conn: sqlite3.Connection):
    """查询白名单统计信息"""
    cursor = conn.cursor()
    cursor.execute("SELECT COUNT(*) FROM uid_whitelist")
    total = cursor.fetchone()[0]
    
    cursor.execute("SELECT source, COUNT(*) FROM uid_whitelist GROUP BY source")
    sources = cursor.fetchall()
    
    print("\n" + "=" * 60)
    print("白名单统计")
    print("=" * 60)
    print(f"总计: {total} 个 16 位 UID")
    print("\n按来源分布:")
    for source, count in sources:
        print(f"  {source}: {count}")


def export_whitelist(conn: sqlite3.Connection, output_file: str = "uid_whitelist.txt"):
    """导出白名单为文本文件"""
    cursor = conn.cursor()
    cursor.execute("SELECT uid FROM uid_whitelist ORDER BY uid")
    uids = cursor.fetchall()
    
    with open(output_file, "w") as f:
        for (uid,) in uids:
            f.write(f"{uid}\n")
    
    print(f"[导出] 已将 {len(uids)} 个 UID 导出到 {output_file}")


def main():
    parser = argparse.ArgumentParser(description="16位 UID 白名单构建器")
    parser.add_argument("--mode", choices=["fans", "comments", "batch", "stats", "export"],
                        default="stats", help="运行模式")
    parser.add_argument("--uid", type=int, help="UP 主 UID (fans 模式)")
    parser.add_argument("--bvid", type=str, help="视频 BV 号 (comments 模式)")
    parser.add_argument("--pages", type=int, default=MAX_PAGES, help="最大页数")
    
    args = parser.parse_args()
    
    # 初始化数据库
    conn = init_db()
    
    if args.mode == "fans":
        if not args.uid:
            print("错误: fans 模式需要指定 --uid")
            return
        collect_from_fans(conn, args.uid, args.pages)
    
    elif args.mode == "comments":
        if not args.bvid:
            print("错误: comments 模式需要指定 --bvid")
            return
        collect_from_comments(conn, args.bvid, args.pages)
    
    elif args.mode == "batch":
        batch_collect(conn)
    
    elif args.mode == "stats":
        query_stats(conn)
    
    elif args.mode == "export":
        export_whitelist(conn)
    
    conn.close()


if __name__ == "__main__":
    main()

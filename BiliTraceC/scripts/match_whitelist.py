#!/usr/bin/env python3
"""
match_whitelist.py - MITM 候选与白名单快速匹配

使用方法:
    python match_whitelist.py candidates_HASH.txt

原理:
    1. 加载白名单数据库中的所有 16 位 UID
    2. 加载 MITM 候选文件
    3. 取交集，命中即为目标
"""

import sqlite3
import sys
from typing import Set

DB_FILE = "uid_whitelist.db"


def load_whitelist() -> Set[int]:
    """加载白名单"""
    try:
        conn = sqlite3.connect(DB_FILE)
        cursor = conn.cursor()
        cursor.execute("SELECT uid FROM uid_whitelist")
        uids = {row[0] for row in cursor.fetchall()}
        conn.close()
        return uids
    except Exception as e:
        print(f"[Error] 无法加载白名单: {e}")
        return set()


def load_candidates(filepath: str) -> Set[int]:
    """加载候选文件"""
    uids = set()
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('#'):
                    try:
                        uids.add(int(line))
                    except ValueError:
                        pass
    except Exception as e:
        print(f"[Error] 无法加载候选文件: {e}")
    return uids


def main():
    if len(sys.argv) < 2:
        print("用法: python match_whitelist.py <candidates_file.txt>")
        print("\n示例: python match_whitelist.py candidates_b611e159.txt")
        sys.exit(1)
    
    candidates_file = sys.argv[1]
    
    print("=" * 60)
    print("MITM 候选白名单匹配器")
    print("=" * 60)
    
    # 加载白名单
    print("\n[1] 加载白名单...")
    whitelist = load_whitelist()
    print(f"    白名单大小: {len(whitelist)} 个 UID")
    
    if not whitelist:
        print("\n[Error] 白名单为空！请先运行 build_uid_whitelist.py 构建白名单")
        sys.exit(1)
    
    # 加载候选
    print(f"\n[2] 加载候选文件: {candidates_file}")
    candidates = load_candidates(candidates_file)
    print(f"    候选数量: {len(candidates)} 个 UID")
    
    # 取交集
    print("\n[3] 计算交集...")
    matches = candidates & whitelist
    
    print("\n" + "=" * 60)
    print("匹配结果")
    print("=" * 60)
    
    if matches:
        print(f"\n✅ 找到 {len(matches)} 个匹配:")
        for uid in sorted(matches):
            print(f"  {uid}")
            print(f"  主页: https://space.bilibili.com/{uid}")
    else:
        print("\n❌ 无匹配")
        print("\n可能原因:")
        print("  1. 目标用户不在已收集的白名单中")
        print("  2. 需要扩充白名单来源（更多 UP 主、更多视频）")
        print("\n建议:")
        print("  运行 'python build_uid_whitelist.py --mode batch' 扩充白名单")


if __name__ == "__main__":
    main()

import zlib
import sys

target = 0x338a39a5
print(f"Searching for ALL UIDs with CRC32 = {hex(target)}")
print("=" * 50)

matches = []
for uid in range(5_000_000_000):
    if uid % 100_000_000 == 0:
        print(f"Progress: {uid:,} / 5,000,000,000", end="\r")
    
    h = zlib.crc32(str(uid).encode())
    if h == target:
        matches.append(uid)
        print(f"\n[MATCH] UID: {uid}")

print(f"\n\n{'=' * 50}")
print(f"Total matches found: {len(matches)}")
for uid in matches:
    print(f"  - {uid} -> https://space.bilibili.com/{uid}")

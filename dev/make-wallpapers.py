#!/usr/bin/env python3
"""Generate a few gradient demo wallpapers (PNG, stdlib only) for the nested dev session and README media.

Usage: dev/make-wallpapers.py OUT_DIR
"""
import struct
import sys
import zlib
from pathlib import Path

W, H = 1920, 1080
# (name, top-left color, bottom-right color)
WALLPAPERS = [
    ("aurora", (16, 44, 66), (88, 28, 120)),
    ("ember", (60, 14, 30), (230, 120, 40)),
    ("lagoon", (8, 60, 70), (40, 160, 150)),
    ("dusk", (30, 24, 70), (200, 90, 120)),
]


def png(path, top, bottom):
    rows = []
    for y in range(H):
        row = bytearray([0])
        for x in range(0, W, 1):
            t = (x / W + y / H) / 2
            row += bytes(int(top[c] + (bottom[c] - top[c]) * t) for c in range(3))
        rows.append(bytes(row))
    raw = zlib.compress(b"".join(rows), 9)

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 2, 0, 0, 0)))
        f.write(chunk(b"IDAT", raw))
        f.write(chunk(b"IEND", b""))


def main():
    out = Path(sys.argv[1])
    out.mkdir(parents=True, exist_ok=True)
    for name, top, bottom in WALLPAPERS:
        target = out / f"{name}.png"
        if not target.exists():
            png(target, top, bottom)


if __name__ == "__main__":
    main()

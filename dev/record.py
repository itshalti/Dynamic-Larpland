#!/usr/bin/env python3
"""Record a screen region to a GIF (used for the README media in docs/media).

grim takes frames at a steady rate, ffmpeg turns them into an optimised GIF.
Usage: dev/record.py OUT.gif X Y W H SECONDS [--fps 30] [--width 900] [--cursor]
"""

import argparse
import os
import shutil
import subprocess
import tempfile
import time


def main():
    p = argparse.ArgumentParser()
    p.add_argument("out")
    p.add_argument("x", type=int)
    p.add_argument("y", type=int)
    p.add_argument("w", type=int)
    p.add_argument("h", type=int)
    p.add_argument("seconds", type=float)
    p.add_argument("--fps", type=int, default=30)
    p.add_argument("--width", type=int, default=900, help="output width (keeps aspect)")
    p.add_argument("--cursor", action="store_true")
    a = p.parse_args()

    tmp = tempfile.mkdtemp(prefix="dl-record-")
    try:
        geometry = f"{a.x},{a.y} {a.w}x{a.h}"
        frames = int(a.seconds * a.fps)
        start = time.monotonic()
        for i in range(frames):
            target = start + i / a.fps
            delay = target - time.monotonic()
            if delay > 0:
                time.sleep(delay)
            cmd = ["grim", "-t", "ppm", "-g", geometry]
            if a.cursor:
                cmd.insert(1, "-c")
            subprocess.run(cmd + [os.path.join(tmp, f"{i:05d}.ppm")], check=True)
        width = min(a.width, a.w)
        filters = (f"fps={a.fps},scale={width}:-1:flags=lanczos,split[s0][s1];"
                   "[s0]palettegen=max_colors=192:stats_mode=diff[p];"
                   "[s1][p]paletteuse=dither=bayer:bayer_scale=4:diff_mode=rectangle")
        subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-framerate", str(a.fps),
                        "-i", os.path.join(tmp, "%05d.ppm"), "-vf", filters, "-loop", "0", a.out],
                       check=True)
        print(f"{a.out}: {frames} frames, {os.path.getsize(a.out) // 1024} KiB")
    finally:
        shutil.rmtree(tmp)


if __name__ == "__main__":
    main()

# Rescales some resources from globedV2 dir to resources/menu

from subprocess import Popen, PIPE
from pathlib import Path

def rescale(src: Path, dst: Path, px: int):
    print(f"Rescaling {src} to {px} pixels wide")

    proc = Popen(["ffmpeg", "-i", str(src.resolve()), "-y", "-vf", f"scale={px}:-1", str(dst.resolve())], stdout=PIPE, stderr=PIPE)
    if proc.wait(1.0) != 0:
        print(f"Failed to rescale {src}")
        if proc.stdout: print(proc.stdout.read())
        if proc.stderr: print(proc.stderr.read())

gv2 = Path("resources/_raw/globedV2")

small = [
    "exit01.png",
    "exit02.png",
    "refresh01.png",
    "search01.png",
    "search02.png",
    "server01.png",
    "server02.png",
    "teams01.png",
    "teams02.png",
]

large = [
    "discord01.png",
    "levels01.png",
    "list01.png",
    "mic01.png",
    "settings01.png",
    "support01.png",
    "support02.png",
    "feature01.png",
    "spit-on-that-thang01.png",
    "moonSendBtn.png",
]

for s in small:
    rescale(gv2 / s, Path("resources/menu") / s, 128)

for s in large:
    rescale(gv2 / s, Path("resources/menu") / s, 256)


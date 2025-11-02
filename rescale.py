# Rescales some resources from globedV2 dir to resources/menu

from subprocess import Popen, PIPE
from pathlib import Path

def rescale(src: Path, dst: Path, px: int):
    print(f"Rescaling {src} to {px} pixels wide")

    used_temp = None
    if dst == src:
        used_temp = "/tmp/rescale_temp.png"

    used_dest = used_temp if used_temp else str(dst)

    proc = Popen(["ffmpeg", "-i", str(src.resolve()), "-y", "-vf", f"scale={px}:-1", used_dest], stdout=PIPE, stderr=PIPE)
    if proc.wait(1.0) != 0:
        print(f"Failed to rescale {src}")
        if proc.stdout: print(proc.stdout.read())
        if proc.stderr: print(proc.stderr.read())
        return

    if used_temp:
        dst.write_bytes(Path(used_temp).read_bytes())

gv2 = Path("resources/_raw/globedV2")
esprites = Path("resources/emotes_sprites")

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

# for s in small:
#     rescale(gv2 / s, Path("resources/menu") / s, 128)

# for s in large:
#     rescale(gv2 / s, Path("resources/menu") / s, 256)

for s in esprites.iterdir():
    if s.suffix.lower() not in [".png", ".jpg", ".jpeg"]:
        continue

    rescale(s, s, 128)
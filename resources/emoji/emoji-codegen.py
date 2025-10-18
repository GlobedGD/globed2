from pathlib import Path
import codecs
import json

data = json.loads((Path(__file__).parent / "emoji-data").read_text())
names = {}

def utf32_to_c(s: str) -> str:
    parts = s.split('-')
    return ''.join(f"\\U{int(p, 16):08x}" for p in parts)

def utf32_to_utf8_c(s: str) -> str:
    chars = ''.join(chr(int(cp, 16)) for cp in s.split('-'))
    utf8_bytes = chars.encode('utf-8')
    return ''.join(f'\\x{b:02x}' for b in utf8_bytes)

for item in data["emojis"]:
    decoded = item["surrogates"]
    codepoints = [f"{ord(c):x}" for c in decoded]
    name = "-".join(codepoints)
    name = name.replace("-fe0f", "")

    names[name] = item["names"]

for name, altnames in names.items():
    print(f'{name}: {", ".join(altnames)}')

print('--------------')

# for file in Path(__file__).parent.iterdir():
#     if not file.name.endswith('.png'):
#         continue

#     cp = file.name[:-4]
#     if cp in names:
#         print(f'{cp}: {", ".join(names[cp])}')
#     else:
#         print(f'{cp}: (no name)')

print("----------------")

second_phase = []
missing = []

for file in Path(__file__).parent.iterdir():
    if not file.name.endswith('.png'):
        continue

    cp = file.name[:-4]
    print(f"        U\"{utf32_to_c(cp)}\"_emoji,")

    if cp in names:
        for alt in names[cp]:
            second_phase.append((alt, cp))
    else:
        missing.append(cp)

print('----------------')

for (alt, cp) in second_phase:
    print(f"        {{ \"{alt}\", u8\"{utf32_to_utf8_c(cp)}\", }},")

print('----------------')

for cp in missing:
    print(f"Missing names for emoji: {cp}")
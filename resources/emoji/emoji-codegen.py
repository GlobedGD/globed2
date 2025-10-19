from pathlib import Path
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

# Add some custom ones that discord doesn't have yet
names["1fae9"] = ["face_with_bags_under_eyes", "exhausted_face", "exhausted"]

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

# print("----------------")

second_phase = []
missing = []

first_str = ""
second_str = ""

paths = list(Path(__file__).parent.iterdir())
paths.sort(key=lambda p: p.name)

for file in paths:
    if not file.name.endswith('.png'):
        continue

    cp = file.name[:-4]
    first_str += f"        U\"{utf32_to_c(cp)}\"_emoji,\n"

    if cp in names:
        for alt in names[cp]:
            second_str += f"        {{ \"{alt}\", u8\"{utf32_to_utf8_c(cp)}\", }},\n"
    else:
        print(f"WARN: Missing names for emoji: {cp}")

source_path = Path(__file__).parent.parent.parent / "src" / "ui" / "Emojis.cpp"
source = source_path.read_text()

def replace_section(source: str, start_marker: str, end_marker: str, new_content: str) -> str:
    start_index = source.index(start_marker) + len(start_marker)
    end_index = source.index(end_marker, start_index)
    return source[:start_index] + "\n" + new_content + source[end_index:]

source = replace_section(source, "// ## BEGIN CODEGEN 1", "// ## END CODEGEN", first_str)
source = replace_section(source, "// ## BEGIN CODEGEN 2", "// ## END CODEGEN", second_str)

source_path.write_text(source)

print(f"Done! Code written to {source_path}")

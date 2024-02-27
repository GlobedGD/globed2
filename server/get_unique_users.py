from pathlib import Path
from datetime import datetime, timedelta
import sys

data = Path(sys.argv[1]).read_text().splitlines()

entries = []

for line in data:
    if "Login successful from " not in line:
        continue

    timestr = line.partition("]")[0][1:]
    username = line.partition("Login successful from ")[2].partition(" (")[0]

    time = datetime.strptime(timestr, "%Y-%m-%d %H:%M:%S.%f")

    entries.append((time, username))

print(f"Total logins: {len(entries)}")

start_time = datetime.now() - timedelta(days=1)
end_time = datetime.now()

filtered = []
for e in entries:
    if e[0] > start_time and e[0] < end_time:
        filtered.append(e)

print(f"Logins during the last 24 hours: {len(filtered)}")

filtered2 = set()
for e in filtered:
    filtered2.add(e[1])

print(f"Unique users: {len(filtered2)}")

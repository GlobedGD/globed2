from pathlib import Path
from datetime import datetime, timedelta
import sys

data = Path(sys.argv[1]).read_text().splitlines()

shift_by_hours = 0
if len(sys.argv) > 2:
    shift_by_hours = int(sys.argv[2])

entries = []
usernames = []

for line in data:
    if "Login successful from " not in line:
        continue

    timestr = line.partition("]")[0][1:]
    username = line.partition("Login successful from ")[2].partition(" (")[0]

    time = datetime.strptime(timestr, "%Y-%m-%d %H:%M:%S.%f")

    entries.append((time, username))

print(f"Total logins: {len(entries)}")

end_time = datetime.now() - timedelta(hours=shift_by_hours)
start_time = end_time - timedelta(days=1)

filtered = [e for e in entries if start_time < e[0] < end_time]

print(f"Logins during the last 24 hours: {len(filtered)}")

unique_users = {e[1] for e in filtered}

print(f"Unique users: {len(unique_users)}")

with open(".unique_users.txt", "w") as f:
    unique_names = {e[1] for e in entries}
    for name in sorted(unique_names):
        f.write(name + "\n")

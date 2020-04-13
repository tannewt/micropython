import os
import sys

new_contents = sys.stdin.read()

fn = sys.argv[1]
old_contents = None
if os.path.isfile(fn):
    with open(fn, "r") as f:
        old_contents = f.read()
else:
    os.makedirs(os.path.basename(fn), exist_ok=True)

if new_contents != old_contents:
    with open(fn, "w") as f:
        f.write(new_contents)

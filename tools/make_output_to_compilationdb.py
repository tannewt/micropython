import subprocess
import sys
import json
import os
import os.path
import shlex

steps = []

cwd = os.getcwd()

for line in sys.stdin:
    if line.startswith("arm-none-eabi-gcc"):
        step = {"directory": cwd,
                "command": line.strip(),
                "file": line.split()[-1]}
        steps.append(step)

extra_includes = subprocess.run(shlex.split("arm-none-eabi-gcc -Wp,-v -x c++ - -fsyntax-only"), input=b"", capture_output=True)

include_paths = [""]
active = False
for line in extra_includes.stderr.decode("utf-8").split("\n"):
    if line == "End of search list.":
        active = False
    if active:
        include_paths.append(os.path.abspath(line.strip()))
    if line == "#include <...> search starts here:":
        active = True

include_paths = " -I".join(include_paths)

for step in steps:
    step["command"] += include_paths
    step["command"] = step["command"].replace("-flto-partition=none", "")
    step["command"] = step["command"].replace("-Wno-error=lto-type-mismatch", "")

with open("compile_commands.json", "w") as f:
    json.dump(steps, f, indent="  ")

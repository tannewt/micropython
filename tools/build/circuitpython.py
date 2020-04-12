import glob
import os
import sys
import ninja
import toml

board = sys.argv[1]
board_directory = glob.glob(f"ports/*/boards/{board}")

if len(board_directory) != 1:
    print("unknown or ambiguous board")
    sys.exit(-1)
board_directory = board_directory[0]

build_directory = f"build-{board}"
os.makedirs(build_directory, exist_ok=True)

# Load the tomls
board_info = toml.load(os.path.join(board_directory, "board.toml"))
mcu_info = toml.load("tools/build/mcu/{}.toml".format(board_info["mcu"]))
mcu_family_info = toml.load("tools/build/mcu_family/{}.toml".format(mcu_info["mcu_family"]))
cpu_info = [toml.load("tools/build/cpu/{}.toml".format(cpu)) for cpu in mcu_family_info["cpu"]]


# Instantiate the rules
entries = []
include_paths = [".", "ports/atmel-samd"]
include_paths.extend(mcu_family_info["extra_include_directories"])
for cpu in cpu_info:
    include_paths.extend(cpu["extra_include_directories"])
include_paths.append(board_directory)

defines = mcu_info["defines"]
defines.extend(mcu_family_info["defines"])
if "external_flash" in board_info:
    defines.append("SPI_FLASH_FILESYSTEM")
    part_numbers = board_info["external_flash"]["part_numbers"]
    defines.append("EXTERNAL_FLASH_DEVICE_COUNT={}".format(len(part_numbers)))
    defines.append("EXTERNAL_FLASH_DEVICES=\"{}\"".format(",".join(part_numbers)))

c = ninja.Compile(build_directory, include_paths, defines)
entries.append(c)
link = ninja.Link(build_directory)
entries.append(link)

outs = ["main.o"]

entries.append(c.build("main.c"))
entries.append(link.build("firmware.elf", outs))

print(board_directory)

with open(f"build-{board}/build.ninja", "w") as f:
    for entry in entries:
        f.write(str(entry))


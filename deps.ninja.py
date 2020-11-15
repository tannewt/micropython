import sys

fn = sys.argv[1]
ninja_out = sys.argv[2]
dep_out = sys.argv[3]

print("deps for", fn)

deps = []

per_port = ["mphalport.h"]

with open(fn, "r") as f:
    for line in f:
        stripped = line.strip()
        if stripped.startswith("#include"):
            if stripped[9] == "<":
                #print(stripped)
                pass
            else:
                try:
                    include = stripped.split("\"", 2)[1]
                except:
                    print(stripped)
                    raise
                if include in per_port or include.startswith("nrfx"):
                    include = "ports/nrf/" + include
                deps.append(include + ".fulldeps")

with open(ninja_out, "w") as f:
    f.write("build ")
    f.write(fn + ".fulldeps")
    if len(deps) > 0:
        f.write(": finalize_deps ")
        f.write(" ".join(deps))
    else:
        f.write(": copy_deps ")
        f.write(fn + ".deps")

    f.write("\n")

with open(dep_out, "w") as f:
    pass

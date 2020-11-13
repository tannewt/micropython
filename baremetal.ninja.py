import pathlib
import subprocess
import os
import toml

root = pathlib.Path(".")


build = root / "build"
build.mkdir(exist_ok=True)
phase1 = build / "phase1"
phase1.mkdir(exist_ok=True)
phase2 = build / "phase2"
phase2.mkdir(exist_ok=True)

phase1_fn = phase1 / "build.ninja"
phase1_ninja = open(phase1_fn, "w")

phase1_ninja.write("rule strip_code\n")
phase1_ninja.write("  command = ag --nonumbers --nocolor \"#(include|define|if|ifdef|ifndef|elif|endif|undef)\" $in > $out || touch $out \n")
phase1_ninja.write("\n")


phase1_ninja.write("rule first_dep_pass\n")
phase1_ninja.write("  command = python ../../deps.ninja.py $in $out\n")
phase1_ninja.write("\n")


phase1_ninja.write("rule cat\n")
phase1_ninja.write("  command = cat $in > $out\n")
phase1_ninja.write("\n")

phase2_fn = phase2 / "build.ninja"
with open(phase2_fn, "w") as f:
    f.write("rule copy_deps\n")
    f.write("  command = cp $in $out\n")
    f.write("\n")
    f.write("rule finalize_deps\n")
    f.write("  command = touch $out\n")
    f.write("\n")
    f.write("subninja fulldeps.ninja\n")

i = 0
ignorelist = set(("./.git",))
for dirpath, dirnames, filenames in os.walk(root):
    # if dirpath in ignorelist:
    #     dirnames[:] = []
    #     continue

    if "baremetal.ninja.toml" in filenames:
        config = toml.load(os.path.join(dirpath, "baremetal.ninja.toml"))
        ignorelist.update([os.path.join(dirpath, d) for d in config["ignore"]])

    current_dir = root / dirpath
    to_remove = []
    for d in dirnames:
        dpath = current_dir / d
        for ignore in ignorelist:
            if dpath.match(ignore):
                to_remove.append(d)
    for d in to_remove:
        dirnames.remove(d)

    dirpath = dirpath.replace(" ", "$ ")

    ninja_deps = []
    for d in dirnames:
        ninja_deps.append("../phase2/{}/fulldeps.ninja".format(os.path.join(dirpath, d)))

    for fn in filenames:
        if not fn.endswith(".c") and not fn.endswith(".h"):
            continue
        i += 1
        fn = os.path.join(dirpath, fn.replace(" ", "$ "))
        phase1_ninja.write("build {}: strip_code ../../{}\n".format(fn, fn))
        ninja_deps.append("../phase2/{}.partninja".format(fn))
        phase1_ninja.write("build ../phase2/{}.partninja ../phase2/{}.deps: first_dep_pass {}\n".format(fn, fn, fn))
        #print(i, fn)

    phase1_ninja.write("build ../phase2/{}/fulldeps.ninja: cat {}\n".format(dirpath, " ".join(ninja_deps)))

phase1_ninja.close()

print(i, "total files")

subprocess.run(["ninja"], cwd=phase1)

print()

print("phase 2")
print("--------------------------------------")

subprocess.run(["ninja"], cwd=phase2)

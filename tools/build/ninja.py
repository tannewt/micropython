class Rule:
    def build(self):
        return ""

class Compile(Rule):
    def __init__(self, build_directory, includes, defines):
        self.build_directory = build_directory
        self.includes = includes
        self.defines = defines

    def __str__(self):
        includes = ["-I../" + i for i in self.includes]
        defines = ["-D" + d for d in self.defines]
        return "rule compile\n  command = arm-none-eabi-gcc {} -c $in -o $out\n\n".format(" ".join(includes + defines))

    def build(self, fn):
        return "build {}: compile {}\n\n".format("objs/" + fn[:-2] + ".o", "../" + fn)

class Link(Rule):
    def __init__(self, build_directory):
        self.build_directory = build_directory

    def __str__(self):
        return "rule link\n  command = arm-none-eabi-gcc -o $out $ldflags $in -Wl,--start-group $libs -Wl,--end-group\n\n"

    def build(self, out, inputs):
        inputs = ["objs/" + x for x in inputs]
        return "build {}: compile {}\n\n".format(out, " ".join(inputs))

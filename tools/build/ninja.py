class Rule:
    def build(self):
        return ""

class Compile(Rule):
    def __init__(self, build_directory, cflags):
        self.build_directory = build_directory
        self.cflags = cflags

    def __str__(self):
        return "rule compile\n  command = arm-none-eabi-gcc {} -c $in -o $out\n\n".format(" ".join(self.cflags))

    def build(self, fn, implicit_deps=[]):
        if implicit_deps:
            implicit_deps = "| " + " ".join(implicit_deps)
        return "build {}: compile {} {}\n\n".format("objs/" + fn[:-2] + ".o", "../" + fn, implicit_deps)

class Link(Rule):
    def __init__(self, build_directory):
        self.build_directory = build_directory

    def __str__(self):
        return "rule link\n  command = arm-none-eabi-gcc -o $out -Wl,-T,$ldfile -Wl,-Map=$out.map $ldflags $in -Wl,--start-group $libs -Wl,--end-group\n\n"

    def build(self, out, inputs, *, ldfile, ldflags="", libs=[]):
        inputs = ["objs/" + x for x in inputs]
        return "build {}: link {}\n\n".format(out, " ".join(inputs))

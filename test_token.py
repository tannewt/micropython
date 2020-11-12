import clang.cindex
import subprocess
import yaml
import pathlib

current_directory = str(pathlib.Path.cwd())


s = subprocess.run(["pp-trace", "--extra-arg=\"-I .\"", "py/mpconfig.h"], capture_output=True)
callbacks = yaml.load(s.stdout)
print(s.stderr)

if_lines = []

for callback in callbacks:
    if callback["Callback"] in ("If", "Elif") and callback["Loc"].startswith(current_directory):
        # print(callback)
        if_lines.append(int(callback["Loc"].split(":")[1]))


index = clang.cindex.Index.create()
translation_unit = index.parse("py/objstr.c", options=3)

# def print_cursor(c, indent=0):
#     if c.location.line == 93:
#         print("  " * indent + str(c.kind) + ": " + c.displayname)
#     for child in c.get_children():
#         print_cursor(child, indent+1)

# print_cursor(translation_unit.cursor)

for include in translation_unit.get_includes():
    print(" " * (include.depth - 1), include.include, include.is_input_file, include.source)

define_deps = set()
last_line = None
for token in translation_unit.get_tokens(extent=translation_unit.cursor.extent):
    line = token.location.line
    if line not in if_lines:
        continue

    if token.kind == clang.cindex.TokenKind.IDENTIFIER:
        define_deps.add(token.spelling)
    # print(token.location.line, token.kind)
    # print(token.spelling)

print(define_deps)

for d in translation_unit.diagnostics:
    print(d)

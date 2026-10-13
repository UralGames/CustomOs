#!/usr/bin/env python3
"""Tests for .github/scripts/build-failure-summary.py (run by CI).

    python3 tests/test-build-failure-summary.py
"""
import os
import subprocess
import sys
import tempfile

SCRIPT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".github", "scripts", "build-failure-summary.py")
ESC = "\x1b[7m"
END = "\x1b[27m"

# From the first failed ISO build (rsync 3.4.3 against Linux 5.4 headers).
RSYNC = f"""{ESC}>>> rsync 3.4.3 Configuring{END}
checking whether to support ACLs... no
configure: WARNING: unrecognized options: --disable-gtk-doc
{ESC}>>> rsync 3.4.3 Building{END}
/usr/bin/make -j4  -C /mnt/customos/output/build/rsync-3.4.3/
x86_64-linux-gcc -I. -O2 -Wall -W -Werror=implicit-function-declaration -c match.c -o match.o
x86_64-linux-gcc -I. -O2 -Wall -W -c syscall.c -o syscall.o
syscall.c:38:10: fatal error: linux/openat2.h: No such file or directory
   38 | #include <linux/openat2.h>
      |          ^~~~~~~~~~~~~~~~~
compilation terminated.
make[1]: *** [Makefile:71: syscall.o] Error 1
make[1]: *** Waiting for unfinished jobs....
make: *** [package/pkg-generic.mk:273: /mnt/customos/output/build/rsync-3.4.3/.stamp_built] Error 2
"""

CASES = [
    ("rsync", RSYNC, "rsync 3.4.3 Building",
     "syscall.c:38:10: fatal error: linux/openat2.h: No such file or directory"),
    ("linker", """>>> foo 1.0 Building
cc -o foo main.o util.o
/usr/bin/ld: main.o: in function `main':
main.c:(.text+0x15): undefined reference to `missing_function'
collect2: error: ld returned 1 exit status
make: *** [Makefile:5: foo] Error 1
""", "foo 1.0 Building", "main.c:(.text+0x15): undefined reference to `missing_function'"),
    ("cmake", """>>> bar 2.0 Configuring
-- The C compiler identification is GNU 14.3.0
CMake Error at CMakeLists.txt:12 (find_package):
  Could not find a package configuration file provided by "Qt6Svg"
-- Configuring incomplete, errors occurred!
""", "bar 2.0 Configuring", "CMake Error at CMakeLists.txt:12 (find_package):"),
    ("oom", """>>> qt6base 6.9.1 Building
[1234/5678] Building CXX object src/gui/painting.o
c++: fatal error: Killed signal terminated program cc1plus
compilation terminated.
ninja: build stopped: subcommand failed.
""", "qt6base 6.9.1 Building", "c++: fatal error: Killed signal terminated program cc1plus"),
    ("ninja", """>>> baz 3.0 Building
[10/20] Compiling C object src/a.c.o
FAILED: src/a.c.o
cc -Isrc -c ../src/a.c -o src/a.c.o
../src/a.c:3:1: error: unknown type name 'foo_t'
ninja: build stopped: subcommand failed.
""", "baz 3.0 Building", "FAILED: src/a.c.o"),
    ("hash", """>>> qux 1.2 Downloading
ERROR: while checking hashes from package/qux/qux.hash
ERROR: qux-1.2.tar.gz has wrong sha256 hash:
""", "qux 1.2 Downloading", "ERROR: while checking hashes from package/qux/qux.hash"),
    ("noerror", """>>>   Executing post-image script board/customos/post-image.sh
post-image: something went wrong
make: *** [Makefile:813: target-post-image] Error 1
""", "Executing post-image script board/customos/post-image.sh",
     "make: *** [Makefile:813: target-post-image] Error 1"),
    ("nothing", """>>> quux 0.1 Building
all good
""", "quux 0.1 Building", None),
]


def run(log_text):
    with tempfile.TemporaryDirectory() as d:
        log, md = os.path.join(d, "build.log"), os.path.join(d, "summary.md")
        with open(log, "w") as f:
            f.write(log_text)
        r = subprocess.run([sys.executable, SCRIPT, log, "--markdown", md, "--annotate"],
                           capture_output=True, text=True, check=True)
        with open(md) as f:
            return r.stdout, f.read()


def main():
    failures = 0
    for name, text, step, error in CASES:
        out, md = run(text)
        ok = f"Failed step: {step}\n" in out and f"**Failed step:** `{step}`" in md
        if error:
            ok = ok and error in md and "**First error:**" in md
            annot = [l for l in out.splitlines() if l.startswith("::error ")]
            ok = ok and len(annot) == 1 and annot[0].endswith("::" + error.replace("%", "%25"))
        else:
            ok = ok and "**First error:**" not in md and "no compiler error found" in out
        ok = ok and "\x1b" not in md
        print(("PASS " if ok else "FAIL ") + name)
        if not ok:
            failures += 1
            print(out)
            print(md)
    # The title of an annotation must not contain ":" or ",".
    out, _ = run(RSYNC)
    title = out.split("::error title=", 1)[1].split("::", 1)[0]
    if ":" in title or "," in title:
        print("FAIL annotation title escaping:", title)
        failures += 1
    else:
        print("PASS annotation title escaping")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())

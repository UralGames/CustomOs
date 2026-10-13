#!/usr/bin/env python3
"""Show the error that stopped a Buildroot build.

    build-failure-summary.py BUILD_LOG [--markdown FILE] [--annotate]

Buildroot announces every package step as ">>> <package> <version> <step>".
The output after the last of those lines belongs to the step that failed. In
it, the *first* error is the cause: with make -jN, the lines after it are
other jobs stopping ("Waiting for unfinished jobs", "Error 2", ...).

Prints the failing step and that error, with a few lines of context, to
stdout (the job log). --markdown appends the same as Markdown to FILE (the
job summary); --annotate also prints a GitHub "::error" annotation, which
shows on the run's page.
"""
import argparse
import collections
import re
import sys

ANSI = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]")
STEP = re.compile(r"^>>>\s+(.+?)\s*$")
STAMP = re.compile(r"/build/([^/\s]+)/\.stamp_(\w+)\]? Error \d+")

# The cause of a failure, in a compiler, linker or build-system message.
PRIMARY = re.compile(
    r"(?:^|[\s:])(?:fatal )?error(?:\[E\d+\])?: "    # gcc, clang, ld, rustc
    r"|undefined reference to |multiple definition of |cannot find -l"
    r"|^configure: error: "
    r"|^CMake Error"
    r"|(?:^|: )ERROR: "                             # meson, Buildroot
    r"|^FAILED: "                                   # ninja
    r"|internal compiler error"
    r"|Killed signal terminated program|virtual memory exhausted|out of memory"
    r"|^Traceback \(most recent call last\)"
    r"|: line \d+: .*(?:command not found|syntax error)"
)
# Only a symptom; used when no PRIMARY line exists.
SECONDARY = re.compile(r"\*\*\* .*Error \d+|^ninja: build stopped|No such file or directory")
# Configure probes print words like "error" while working normally.
NOISE = re.compile(r"^checking |^configure: WARNING")

CONTEXT_BEFORE = 3
CONTEXT_AFTER = 15
TAIL = 40
MAX_LINE = 400


def clean(line):
    line = ANSI.sub("", line.rstrip("\r\n"))
    return line if len(line) <= MAX_LINE else line[:MAX_LINE] + " …"


def analyse(path):
    step, segment, stamp = None, [], None
    tail = collections.deque(maxlen=TAIL)
    with open(path, "rb") as f:
        for raw in f:
            line = clean(raw.decode("utf-8", "replace"))
            m = STEP.match(line)
            if m:
                step, segment = m.group(1), []
                continue
            segment.append(line)
            tail.append(line)
            s = STAMP.search(line)
            if s:
                stamp = s.groups()
    first = next((i for i, l in enumerate(segment) if PRIMARY.search(l) and not NOISE.match(l)), None)
    if first is None:
        first = next((i for i, l in enumerate(segment) if SECONDARY.search(l) and not NOISE.match(l)), None)
    if step is None and stamp:
        step = f"{stamp[0]} ({stamp[1]})"
    excerpt = segment[max(0, first - CONTEXT_BEFORE):first + CONTEXT_AFTER + 1] if first is not None else []
    return {
        "step": step or "unknown step",
        "error": segment[first].strip() if first is not None else None,
        "excerpt": excerpt,
        "tail": list(tail),
    }


def fence(lines):
    body = "\n".join(lines).replace("````", "` ` ` `")
    return f"````text\n{body}\n````"


def escape_data(text):
    return text.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")


def escape_property(text):
    return escape_data(text).replace(":", "%3A").replace(",", "%2C")


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("log")
    p.add_argument("--markdown", help="append a Markdown report to this file")
    p.add_argument("--annotate", action="store_true", help="print a GitHub ::error annotation")
    a = p.parse_args()
    try:
        r = analyse(a.log)
    except OSError as e:
        print(f"build-failure-summary: {e}", file=sys.stderr)
        return 1

    print(f"Failed step: {r['step']}")
    if r["error"]:
        print("First error:")
        print("\n".join("    " + l for l in r["excerpt"]))
    else:
        print("No compiler or build-system error found; last lines of the log:")
        print("\n".join("    " + l for l in r["tail"]))

    if a.markdown:
        out = [f"**Failed step:** `{r['step']}`", ""]
        if r["error"]:
            out += ["**First error:**", "", fence(r["excerpt"]), ""]
        out += ["<details><summary>Last lines of the build log</summary>", "", fence(r["tail"]), "", "</details>", ""]
        with open(a.markdown, "a", encoding="utf-8") as f:
            f.write("\n".join(out) + "\n")

    if a.annotate:
        title = f"Build failed: {r['step']}"
        message = r["error"] or "no compiler error found; see the job summary"
        print(f"::error title={escape_property(title)}::{escape_data(message)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

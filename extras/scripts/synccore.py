"""Copies the Blaeck core from a BlaeckSerial checkout into this library.

BlaeckCore.h and BlaeckCore.cpp are written and changed only in BlaeckSerial. This copies
them byte for byte and records the BlaeckSerial commit they came from in
extras/core-source.txt, which CI checks the copy against.

Usage:
  python extras/scripts/synccore.py                      # from ../BlaeckSerial
  python extras/scripts/synccore.py path/to/BlaeckSerial
"""
import os
import subprocess
import sys

FILES = ["src/BlaeckCore.h", "src/BlaeckCore.cpp"]

here = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
source = os.path.abspath(sys.argv[1]) if len(sys.argv) > 1 else os.path.join(here, "..", "BlaeckSerial")


def git(*args):
    return subprocess.run(["git", "-C", source] + list(args), check=True,
                          capture_output=True, text=True).stdout.strip()


# A copy of uncommitted work would record a commit that doesn't contain it, and CI would
# then fail on a mismatch nobody can reproduce.
dirty = git("status", "--porcelain", "--", *FILES)
if dirty:
    sys.exit("BlaeckSerial has uncommitted changes in the core:\n" + dirty)

commit = git("rev-parse", "HEAD")
for f in FILES:
    with open(os.path.join(source, f), "rb") as src:
        data = src.read()
    with open(os.path.join(here, f), "wb") as dst:
        dst.write(data)

with open(os.path.join(here, "extras", "core-source.txt"), "w", newline="\r\n") as out:
    out.write(commit + "\n")

print("Copied the core from BlaeckSerial " + commit[:7])

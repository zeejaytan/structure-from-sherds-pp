#!/usr/bin/env python3
"""mk_inc.py SRC OUT POT|NONE [--append BLOCK]

Ticket 305: build a per-pot include root from the SIF's pristine class/data_path.h.
Every '#define POT_<X>' line is commented out and exactly ONE is activated (the
'pot' argument), or none at all for 'NONE' (the Juglet recipe, which appends its
own block).  The result is passed to arm67.sh as -I<incdir> so class/data_path.h
resolves to it before /opt/sfs-pristine/class/.

Fails (exit 3) unless the output carries exactly one active POT_ define (or none
for NONE).  Nothing outside OUT is written.
"""
import argparse
import re
import sys

PAT = re.compile(r"^\s*(?://\s*)?#define\s+(POT_[A-J])(?:\s|$)")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("out")
    ap.add_argument("pot", help="A..J, or NONE (activate nothing)")
    ap.add_argument("--append", default="", help="block file appended verbatim")
    a = ap.parse_args()

    with open(a.src) as fh:
        src = fh.read().splitlines(True)
    out = []
    act = 0
    for ln in src:
        m = PAT.match(ln)
        if not m:
            out.append(ln)
            continue
        name = m.group(1)
        if a.pot != "NONE" and name == "POT_" + a.pot and act == 0:
            out.append("#define %s\n" % name)
            act += 1
        else:
            out.append("// #define %s\n" % name)
    txt = "".join(out)
    if a.append:
        block = open(a.append).read()
        if not txt.endswith("\n"):
            txt += "\n"
        txt += block
        if not txt.endswith("\n"):
            txt += "\n"
    with open(a.out, "w") as fh:
        fh.write(txt)
    nactive = len(re.findall(r"^#define POT_[A-J]$", txt, re.M))
    print("mk_inc: %s -> %s : pot=%s append=%s (%d active POT_ define line(s))"
          % (a.src, a.out, a.pot, a.append or "-", nactive))
    want = 0 if a.pot == "NONE" else 1
    if nactive != want:
        print("!!!! mk_inc: expected %d active POT_ define(s), found %d" % (want, nactive))
        return 3
    return 0


if __name__ == "__main__":
    sys.exit(main())

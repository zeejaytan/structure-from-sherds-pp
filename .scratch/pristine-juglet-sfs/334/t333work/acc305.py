#!/usr/bin/env python3
"""acc305.py RESULT_DIR LABEL OUT_TSV

Reads the assembler's own '1. Acc.txt' (class/data_structure.cpp:970 CountResult)
and writes one tab row: label, time_s, sherd k/n, edge k/n, the two raw lines.
The file carries TWO lines labelled 'Sherd accuracy :' -- the first is per-sherd
(a sherd counts if >=1 of its GT edges is within the bar), the second is the
edge tally divided by 2.  Both are recorded verbatim, first=sherd, second=edge.
"""
import os
import re
import sys


def main():
    res, label, out_tsv = sys.argv[1], sys.argv[2], sys.argv[3]
    p = os.path.join(res, "1. Acc.txt")
    rows = []
    if not os.path.isfile(p):
        rows.append((label, "MISSING", "", "", ""))
    else:
        txt = open(p, errors="replace").read()
        t = re.search(r"Time\s*:\s*([0-9.]+)", txt)
        acc = re.findall(r"Sherd accuracy\s*:\s*(\d+)\s*/\s*(\d+)", txt)
        sherd = "%s/%s" % acc[0] if len(acc) > 0 else ""
        edge = "%s/%s" % acc[1] if len(acc) > 1 else ""
        raw = " | ".join(x.strip() for x in txt.splitlines() if "accuracy" in x)
        rows.append((label, t.group(1) if t else "", sherd, edge, raw))
    with open(out_tsv, "a") as fh:
        for r in rows:
            fh.write("\t".join(str(x) for x in r) + "\n")
    print("ACC %s %s %s" % (label, rows[0][2], rows[0][3]))
    return 0


if __name__ == "__main__":
    sys.exit(main())

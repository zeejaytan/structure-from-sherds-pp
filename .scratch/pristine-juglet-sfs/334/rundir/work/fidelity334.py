#!/usr/bin/env python3
"""fidelity334.py -- compare the diag's admitted list (SURVIVOR lines) with ticket 333's
[T333-PAIR] list for the same tree. They must be the same multiset of (ordered pair, score).

  --ref  file with 333's lines, prefix already stripped ("x y size=.. score=.. ...")
  --log  the diag log
  --out  report file
exit 0 = identical counts, pairs and scores; 1 = mismatch; 2 = could not parse.
"""
import argparse, re, sys
from collections import Counter

KV = re.compile(r"(\w+)=([-0-9.eE+]+)")

def ref_rows(path):
    rows = []
    for ln in open(path, errors="ignore"):
        f = ln.split()
        if len(f) < 2:
            continue
        try:
            x, y = int(f[0]), int(f[1])
        except ValueError:
            continue
        kv = dict(KV.findall(ln))
        rows.append((y, x, float(kv.get("score", "nan")), int(kv.get("size", "0"))))
    return rows

def surv_rows(path):
    rows = []
    for ln in open(path, errors="ignore"):
        if not ln.startswith("SURVIVOR\t"):
            continue
        f = ln.rstrip("\n").split("\t")
        if len(f) < 7 or f[1] == "idx":
            continue
        rows.append((int(f[2]), int(f[3]), float(f[5]), None))   # y, x, score, size-not-printed
    return rows

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ref", required=True)
    ap.add_argument("--log", required=True)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    R, S = ref_rows(a.ref), surv_rows(a.log)
    if not R:
        print("no reference rows in %s" % a.ref)
        return 2
    out = []
    out.append("reference %s : %d admitted lines" % (a.ref, len(R)))
    out.append("diag      %s : %d SURVIVOR lines" % (a.log, len(S)))
    pr = Counter((y, x) for y, x, _, _ in R)
    ps = Counter((y, x) for y, x, _, _ in S)
    out.append("unique pairs: reference %d, diag %d" % (len(pr), len(ps)))
    out.append("ordered (y,x) pairs only in reference: %s" % sorted(set(pr) - set(ps)))
    out.append("ordered (y,x) pairs only in diag     : %s" % sorted(set(ps) - set(pr)))
    for k in sorted(set(pr) | set(ps)):
        if pr[k] != ps[k]:
            out.append("  count differs for %s: reference %d, diag %d" % (k, pr[k], ps[k]))
    sr = Counter(round(s, 6) for _, _, s, _ in R)
    ss = Counter(round(s, 6) for _, _, s, _ in S)
    if sr != ss:
        out.append("score multiset differs:")
        for k in sorted(set(sr) | set(ss)):
            if sr[k] != ss[k]:
                out.append("  score %.6f : reference %d, diag %d" % (k, sr[k], ss[k]))
    else:
        out.append("score multiset: identical (%d distinct scores)" % len(sr))
    ok = (len(R) == len(S) and pr == ps and sr == ss)
    out.append("FIDELITY: %s" % ("EXACT" if ok else "MISMATCH"))
    txt = "\n".join(out)
    open(a.out, "w").write(txt + "\n")
    print(txt)
    return 0 if ok else 1

if __name__ == "__main__":
    sys.exit(main())

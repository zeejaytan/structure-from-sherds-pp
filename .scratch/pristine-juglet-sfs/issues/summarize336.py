#!/usr/bin/env python3
"""Ticket 336: merge per-arm seams9.tsv (+ summary.json) into ticket tables.

Usage:
  python3 summarize336.py LBL=PATH.seams9.tsv[,PATH.summary.json] [LBL=... ] [--acc ACCFILE]

Class-D rows are annotated with the reason the pose is not a decided one:
unplaced (a piece is not in the winning state), cross_cluster (the two pieces sit
in different graphs, so no stage ever set their relative pose), in_cluster.
"""
import sys, os, json

SEAMS9 = [(1,2),(1,4),(1,8),(1,9),(2,5),(2,9),(3,5),(6,7),(7,9)]

def read_tsv(path):
    rows = []
    with open(path) as f:
        head = f.readline().rstrip("\n").split("\t")
        for line in f:
            line = line.rstrip("\n")
            if line:
                rows.append(dict(zip(head, line.split("\t"))))
    return rows

def fnum(x, nd=2):
    try:
        return ("%%.%df" % nd) % float(x)
    except Exception:
        return str(x)

def d_reason(r, piece_graph):
    u, v = (int(x) for x in r["seam"].split("-"))
    if not piece_graph:
        return ""
    if u not in piece_graph or v not in piece_graph:
        return "unplaced"
    if piece_graph[u] != piece_graph[v]:
        return "cross_cluster"
    return "in_cluster"

def main(argv):
    arms, accf = [], None
    i = 0
    while i < len(argv):
        a = argv[i]
        if a == "--acc":
            accf = argv[i + 1]; i += 2; continue
        if "=" in a:
            lbl, p = a.split("=", 1)
            summ = None
            if "," in p:
                p, summ = p.split(",", 1)
            arms.append((lbl, p, summ))
        i += 1
    if not arms and not accf:
        print(__doc__); return 2

    data = []
    for lbl, p, summ in arms:
        if not os.path.exists(p):
            print("MISSING %s (%s)" % (p, lbl)); continue
        piece_graph = {}
        if summ and os.path.exists(summ):
            js = json.load(open(summ))
            for c in js.get("clusters", []):
                for n in c["nodes"]:
                    piece_graph[int(n)] = int(c["graph"])
        d = {}
        for r in read_tsv(p):
            d[r["seam"]] = r
        data.append((lbl, d, piece_graph))

    if data:
        print("### Class by seam and arm\n")
        print("| seam | " + " | ".join(a[0] for a in data) + " |")
        print("|" + "---|" * (len(data) + 1))
        for (u, v) in SEAMS9:
            cells = []
            for _lbl, d, pg in data:
                r = d.get("%d-%d" % (u, v))
                if r is None:
                    cells.append("-")
                elif r["class_"] == "D":
                    dr = d_reason(r, pg)
                    cells.append("**D** %s%s" % (r["deciding"], " (%s)" % dr if dr else ""))
                elif r["class_"] in ("A", "B", "C"):
                    cells.append("**%s** %s" % (r["class_"], r["deciding"]))
                else:
                    cells.append("%s %s" % (r["class_"], r["deciding"]))
            print("| %d-%d | " % (u, v) + " | ".join(cells) + " |")

        print("\n### Counts and class-D stages\n")
        print("| arm | A | B | C | D | D stages | D reasons |")
        print("|---|---|---|---|---|---|---|")
        for lbl, d, pg in data:
            c = {}
            dh, dr_ = {}, {}
            for r in d.values():
                k = r["class_"]
                c[k] = c.get(k, 0) + 1
                if k == "D":
                    dh[r["deciding"]] = dh.get(r["deciding"], 0) + 1
                    rr = d_reason(r, pg)
                    if rr:
                        dr_[rr] = dr_.get(rr, 0) + 1
            print("| %s | %d | %d | %d | %d | %s | %s |" % (
                lbl, c.get("A", 0), c.get("B", 0), c.get("C", 0), c.get("D", 0),
                ", ".join("%s=%d" % kv for kv in sorted(dh.items())) or "-",
                ", ".join("%s=%d" % kv for kv in sorted(dr_.items())) or "-"))

        print("\n### Detail (final pose error, matched candidate, nearest alternative)\n")
        for lbl, d, pg in data:
            print("**%s**\n" % lbl)
            print("| seam | class | deciding | rot err deg | trans err mm | matched cand | cand rot diff | nearest alt | alt rot diff | gate / margin | first phase |")
            print("|---|---|---|---|---|---|---|---|---|---|---|")
            for (u, v) in SEAMS9:
                r = d.get("%d-%d" % (u, v))
                if r is None:
                    print("| %d-%d | - | - | - | - | - | - | - | - | - | - |" % (u, v)); continue
                dec = r["deciding"]
                if r["class_"] == "D":
                    rr = d_reason(r, pg)
                    if rr:
                        dec = "%s (%s)" % (dec, rr)
                print("| %d-%d | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s |" % (
                    u, v, r["class_"], dec, fnum(r["rot_err"]), fnum(r["trans_err"]),
                    r["matched_idx"], fnum(r["matched_rot"]), r["nearest_idx"],
                    fnum(r["nearest_rot"]), (r["gate"] + " " + r["gate_margin"]).strip(),
                    r["first_phase"]))
            for s, n in [(r["seam"], r["note"]) for r in d.values() if r.get("note")]:
                print("- seam %s note: %s" % (s, n))
            print("")

    if accf:
        print("\n### Scores\n")
        print("| arm | trace | rc | acc |")
        print("|---|---|---|---|")
        for line in open(accf):
            line = line.strip()
            if not line:
                continue
            parts = line.split()
            arm = parts[0]
            tr = next((p.split("=", 1)[1] for p in parts if p.startswith("trace=")), "?")
            rc = next((p.split("=", 1)[1] for p in parts if p.startswith("rc=")), "?")
            acc = line.split("acc=", 1)[1] if "acc=" in line else "?"
            print("| %s | %s | %s | %s |" % (arm, tr, rc, acc))
    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

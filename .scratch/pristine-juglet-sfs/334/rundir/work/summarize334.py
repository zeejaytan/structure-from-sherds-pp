#!/usr/bin/env python3
"""summarize334.py -- turn one diag334 log into the ticket-334 tables.

T334PAIR column order (1-based after the tag):
  1 u  2 v  3 n_cand  4 closest_idx  5 closest_rot  6 closest_t  7 closest_rms  8 closest_ncor
  9 closest_score  10 closest_axis  11 closest_erase  12 winner_idx  13 winner_rot  14 winner_t
  15 winner_score  16 winner_axis  17 winner_erase  18 n_surv

Reads the DIAG log (CAND / WALL / DISP / T334PAIR lines) and writes a text report:
  * the nine-seam table (candidates, closest to GT, winner, who erased the closest)
  * per seam, the rotation histogram of every candidate and the right-way-up list
    (right-way-up = rot <= 15 deg AND translation <= 10 mm, the ticket's definition)
  * per seam, the wall split of the closest candidate's matched dots
  * the axis sign-blindness numbers for the admitted winners
"""
import argparse, math, sys
from collections import Counter

def rows(path, tag):
    for ln in open(path, errors="ignore"):
        if ln.startswith(tag + "\t"):
            yield ln.rstrip("\n").split("\t")

def fnum(x):
    try:
        return float(x)
    except ValueError:
        return float("nan")

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--run", required=True)
    a = ap.parse_args()
    log = a.run + "/logs/diag_juglet.log"
    out = []
    cand, wall, disp, pairs = {}, {}, {}, []
    for f in rows(log, "CAND"):
        if f[1] == "idx":
            continue
        cand[int(f[1])] = dict(idx=int(f[1]), y=int(f[2]), x=int(f[3]), gt=int(f[4]), size=int(f[5]),
                               ncor=int(f[10]), score=fnum(f[12]), sdist=fnum(f[13]), snorm=fnum(f[14]),
                               axis_raw=fnum(f[15]), axis=fnum(f[16]), overlap=int(f[17]), area=fnum(f[18]),
                               rot=fnum(f[19]), tn=fnum(f[20]), rms=fnum(f[21]))
    for f in rows(log, "WALL"):
        if f[1] == "idx":
            continue
        wall[int(f[1])] = dict(rot=fnum(f[4]), tn=fnum(f[5]), ncor=int(f[6]), gap=fnum(f[7]),
                               walls=f[8], seq=f[9] if len(f) > 9 else "-")
    for f in rows(log, "DISP"):
        if f[1] == "idx":
            continue
        disp[int(f[1])] = dict(erase=f[22], also=f[23], merged=f[24], score2=fnum(f[15]))
    for f in rows(log, "T334PAIR"):
        if f[1] == "u":
            continue
        pairs.append(f)

    out.append("candidates: %d ; on a true seam: %d" % (len(cand), sum(1 for c in cand.values() if c["gt"])))
    out.append("dispositions: %s" % dict(Counter(d["erase"] for d in disp.values())))
    mis = []
    for idx, d in disp.items():
        c = cand.get(idx)
        if c is None:
            continue
        if abs(d["score2"] - c["score"]) > 1e-6:
            mis.append((idx, c["y"], c["x"], c["score"], d["score2"]))
    out.append("score self-check (score2 recomputed vs Icp's score_): %d of %d differ" % (len(mis), len(disp)))
    for m in mis[:20]:
        out.append("   differs: idx=%d y-x=%d-%d score_=%.6f score2=%.6f" % m)
    out.append("")
    out.append("== nine true seams (T334PAIR) ==")
    out.append("\t".join(["u-v", "n_cand", "closest_idx", "closest_rot", "closest_t_mm", "closest_rms",
                          "closest_ncor", "closest_score", "closest_axis", "closest_erase",
                          "winner_idx", "winner_rot", "winner_t_mm", "winner_score", "winner_axis",
                          "winner_erase", "n_surv", "rightway_cands"]))
    for f in pairs:
        u, v = int(f[1]), int(f[2])
        mine = [c for c in cand.values() if (c["y"], c["x"]) in ((u, v), (v, u))]
        rightway = [c for c in mine if c["rot"] <= 15.0 and c["tn"] <= 10.0]
        out.append("\t".join([f"{u}-{v}"] + f[3:19] + [str(len(rightway))]))
        hist = Counter()
        for c in mine:
            r = c["rot"]
            b = "<15" if r < 15 else "15-45" if r < 45 else "45-90" if r < 90 else "90-135" if r < 135 else "135-180"
            hist[b] += 1
        out.append("    rotation histogram: " + ", ".join("%s=%d" % (k, hist[k]) for k in ["<15", "15-45", "45-90", "90-135", "135-180"] if hist[k]))
        for c in sorted(mine, key=lambda c: (c["rot"], c["tn"]))[:3]:
            d = disp.get(c["idx"], {})
            out.append("    near: idx=%d y-x=%d-%d rot=%.2f t=%.2f rms=%.2f ncor=%d score=%.6f score2=%.6f axis=%.2f erase=%s also=%s"
                       % (c["idx"], c["y"], c["x"], c["rot"], c["tn"], c["rms"], c["ncor"], c["score"],
                          d.get("score2", float("nan")), c["axis"], d.get("erase", "?"), d.get("also", "")))
        for c in rightway:
            d = disp.get(c["idx"], {})
            out.append("    RIGHT-WAY-UP: idx=%d y-x=%d-%d rot=%.2f t=%.2f rms=%.2f ncor=%d score=%.6f axis=%.2f erase=%s also=%s"
                       % (c["idx"], c["y"], c["x"], c["rot"], c["tn"], c["rms"], c["ncor"], c["score"], c["axis"],
                          d.get("erase", "?"), d.get("also", "")))
        if f[4] != "-":
            ci = int(f[4])
            if ci in wall:
                w = wall[ci]
                out.append("    closest walls: ncor=%d gap=%.3f %s" % (w["ncor"], w["gap"], w["walls"]))
                out.append("    closest wall seq: %s" % w["seq"])
    out.append("")
    out.append("== axis test on the admitted winner of every true seam ==")
    for f in pairs:
        if f[12] == "-":
            continue
        widx = int(f[12]); wrot = fnum(f[13]); wt = fnum(f[14]); wax = fnum(f[16])
        craw = cand.get(widx, {}).get("axis_raw", float("nan"))
        out.append("  %s-%s winner idx=%d rot=%.2f t=%.2f score=%s axis_folded=%.1f deg axis_raw=%.1f deg -> %s"
                   % (f[1], f[2], widx, wrot, wt, f[15], math.degrees(wax), craw,
                      "UPSIDE DOWN (rot>90)" if wrot > 90 else "right way up"))
    txt = "\n".join(out)
    print(txt)
    open(a.run + "/logs/summary334.txt", "w").write(txt + "\n")
    return 0

if __name__ == "__main__":
    sys.exit(main())

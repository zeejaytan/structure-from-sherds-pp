#!/usr/bin/env python3
"""analysis334.py -- ticket 334: the nine-seam table, the classification, the axis numbers.

  --log   logs/diag_juglet.log from the run dir
  --ref   work/ref333_arm1.txt (333's arm-1 admitted list)
  --clusters work/ref333_clusters_arm1.txt
"""
import argparse, math, sys
from collections import Counter, defaultdict

def rows(path, tag):
    for ln in open(path, errors="ignore"):
        if ln.startswith(tag + "\t"):
            yield ln.rstrip("\n").split("\t")

def fnum(x):
    try:
        return float(x)
    except (TypeError, ValueError):
        return float("nan")

ap = argparse.ArgumentParser()
ap.add_argument("--log", required=True)
ap.add_argument("--ref", required=True)
ap.add_argument("--clusters", default="")
ap.add_argument("--nine", default="1-2,1-4,1-8,1-9,2-5,2-9,3-5,6-7,7-9",
                help="the nine touching seams (ticket 329: graph edges with answer-key gap_min < ~2 mm)")
a = ap.parse_args()
NINE = []
for tok in a.nine.split(","):
    u, v = tok.split("-")
    NINE.append((int(u), int(v)))
NINESET = set(NINE)

cand, wall, disp, pairs, piece = {}, {}, {}, [], {}
for f in rows(a.log, "PIECE"):
    if f[1] == "piece":
        continue
    piece[int(f[1])] = dict(npts=f[2], r0=fnum(f[11]), r1=fnum(f[12]), side=f[13])
for f in rows(a.log, "CAND"):
    if f[1] == "idx":
        continue
    cand[int(f[1])] = dict(idx=int(f[1]), y=int(f[2]), x=int(f[3]), gt=int(f[4]), size=int(f[5]),
                           sx=int(f[6]), sy=int(f[7]), ex=int(f[8]), ey=int(f[9]), ncor=int(f[10]),
                           truefrac=fnum(f[11]), score=fnum(f[12]), sdist=fnum(f[13]), snorm=fnum(f[14]),
                           axis_raw=fnum(f[15]), axis=fnum(f[16]), overlap=int(f[17]), area=fnum(f[18]),
                           rot=fnum(f[19]), tn=fnum(f[20]), rms=fnum(f[21]))
for f in rows(a.log, "WALL"):
    if f[1] == "idx":
        continue
    wall[int(f[1])] = dict(rot=fnum(f[4]), tn=fnum(f[5]), ncor=int(f[6]), gap=fnum(f[7]), walls=f[8],
                           seq=f[9] if len(f) > 9 else "-")
for f in rows(a.log, "DISP"):
    if f[1] == "idx":
        continue
    disp[int(f[1])] = dict(erase=f[22], also=f[23], merged=f[24], score2=fnum(f[15]))
for f in rows(a.log, "T334PAIR"):
    if f[1] == "u":
        continue
    pairs.append(f)

print("== 1. fidelity ==")
ref = []
for ln in open(a.ref, errors="ignore"):
    g = ln.split()
    if len(g) >= 2:
        ref.append((int(g[1]), int(g[0])))
surv = [(c["y"], c["x"]) for c in cand.values() if disp.get(c["idx"], {}).get("erase") == "survived"]
print("reference lines %d (unique pairs %d) ; diag SURVIVOR lines %d (unique pairs %d)"
      % (len(ref), len(set(ref)), len(surv), len(set(surv))))
print("pair multiset identical: %s ; missing %s ; extra %s"
      % (Counter(ref) == Counter(surv), sorted(set(ref) - set(surv)), sorted(set(surv) - set(ref))))
print("candidates %d ; score self-check mismatches %d"
      % (len(cand), sum(1 for i, d in disp.items() if i in cand and abs(d["score2"] - cand[i]["score"]) > 1e-6)))

print()
print("== 2. wall convention (Surface_0) vs ticket 328 ==")
print("piece  r(Surface_0)  r(Surface_1)  instrument  ticket328")
t328 = {2: "outer", 6: "outer", 9: "outer"}
for i in sorted(piece):
    mine = "outer" if piece[i]["side"] == "sur0=OUTER" else "inner"
    print("%5d  %12.3f  %12.3f  %10s  %9s  %s" % (i, piece[i]["r0"], piece[i]["r1"], mine,
          t328.get(i, "inner"), "agree" if mine == t328.get(i, "inner") else "DISAGREE"))

print()
print("== 2b. answer-key gap per graph edge (GTPAIR), to confirm which nine touch ==")
gp = {}
for f in rows(a.log, "GTPAIR"):
    if f[1] == "y":
        continue
    gp[(int(f[1]), int(f[2]))] = fnum(f[3])
for k in sorted(gp, key=lambda k: gp[k]):
    print("  %d-%d gap_min=%.3f mm  %s" % (k[0], k[1], gp[k], "TOUCHING" if k in NINESET else ""))
print("  nine from ticket 329: %s ; under 2 mm: %s"
      % (sorted(NINESET), sorted(k for k in gp if gp[k] < 2.0)))

print()
print("== 3. the nine true seams ==")
hdr = ("seam n_cand | closest: idx rot t rms ncor score axis erase | winner: idx rot t score axis erase | "
       "rw(<=15,<=10) rw_surv")
print(hdr)
for f in [f for f in pairs if (int(f[1]), int(f[2])) in NINESET]:
    u, v = int(f[1]), int(f[2])
    mine = [c for c in cand.values() if (c["y"], c["x"]) in ((u, v), (v, u))]
    rw = [c for c in mine if c["rot"] <= 15.0 and c["tn"] <= 10.0]
    rw_s = [c for c in rw if disp.get(c["idx"], {}).get("erase") == "survived"]
    print("%d-%d %6d | %s | %s | %d %d"
          % (u, v, len(mine),
             " ".join(f[4:11]) if f[4] != "-" else "none",
             " ".join([f[12], f[13], f[14], f[15], f[16], f[17]]) if f[12] != "-" else "none",
             len(rw), len(rw_s)))

print()
print("== 4. classification per seam (pre-registered readings) ==")
for f in [f for f in pairs if (int(f[1]), int(f[2])) in NINESET]:
    u, v = int(f[1]), int(f[2])
    mine = [c for c in cand.values() if (c["y"], c["x"]) in ((u, v), (v, u))]
    rw = [c for c in mine if c["rot"] <= 15.0 and c["tn"] <= 10.0]
    rw_s = [c for c in rw if disp.get(c["idx"], {}).get("erase") == "survived"]
    win = cand.get(int(f[12])) if f[12] != "-" else None
    print("-- seam %d-%d: %d candidates, %d right-way-up, %d right-way-up surviving"
          % (u, v, len(mine), len(rw), len(rw_s)))
    for c in rw:
        d = disp.get(c["idx"], {})
        print("     rw idx=%d rot=%.2f t=%.2f rms=%.2f ncor=%d score=%.6f (dist %.3f norm %.3f) axis=%.2f erase=%s also=%s"
              % (c["idx"], c["rot"], c["tn"], c["rms"], c["ncor"], c["score"], c["sdist"], c["snorm"],
                 c["axis"], d.get("erase"), d.get("also")))
    if win:
        print("     winner idx=%d rot=%.2f t=%.2f ncor=%d score=%.6f (dist %.3f norm %.3f) axis=%.2f overlap=%d"
              % (win["idx"], win["rot"], win["tn"], win["ncor"], win["score"], win["sdist"], win["snorm"],
                 win["axis"], win["overlap"]))
    if rw_s and win and win["rot"] > 90:
        best = min(rw_s, key=lambda c: c["score"])
        print("     READING 1: proposed and surviving but outscored. best rw score %.6f vs winner %.6f "
              "(delta %.6f); ncor %d vs %d, dist %.3f vs %.3f, norm %.3f vs %.3f"
              % (best["score"], win["score"], win["score"] - best["score"], best["ncor"], win["ncor"],
                 best["sdist"], win["sdist"], best["snorm"], win["snorm"]))
    elif rw_s:
        print("     proposed and surviving; no flipped winner on this seam")
    elif rw:
        gates = Counter()
        for c in rw:
            gates[disp.get(c["idx"], {}).get("erase", "?")] += 1
        print("     READING 2: proposed but erased by a gate: %s" % dict(gates))
        for c in rw:
            d = disp.get(c["idx"], {})
            print("        idx=%d rot=%.2f t=%.2f score=%.6f axis=%.2f erased_by=%s also=%s merged_into=%s"
                  % (c["idx"], c["rot"], c["tn"], c["score"], c["axis"], d.get("erase"), d.get("also"), d.get("merged")))
    else:
        print("     READING 3: never proposed (no candidate within 15 deg / 10 mm), closest rot %.1f deg"
              % (min([c['rot'] for c in mine], default=float('nan'))))
        if f[4] != "-":
            c = cand[int(f[4])]
            print("        closest idx=%d seed sx..ex=%d-%d sy..ey=%d-%d size=%d lcs_truefrac=%.3f ncor=%d gap=%.3f"
                  % (c["idx"], c["sx"], c["ex"], c["sy"], c["ey"], c["size"], c["truefrac"], c["ncor"],
                     wall.get(c["idx"], {}).get("gap", float("nan"))))
            w = wall.get(c["idx"])
            if w:
                print("        closest walls %s" % w["walls"])
                print("        closest wall seq %s" % w["seq"])

print()
print("== 5. axis sign-blindness ==")
flipped = [c for c in cand.values() if disp.get(c["idx"], {}).get("erase") == "survived" and c["rot"] > 90]
print("admitted (surviving) candidates that are upside down vs GT: %d" % len(flipped))
for c in sorted(flipped, key=lambda c: c["rot"]):
    print("  idx=%d y-x=%d-%d rot=%.2f axis_raw=%.1f deg axis_folded=%.2f rad (%.2f deg) ncor=%d score=%.6f"
          % (c["idx"], c["y"], c["x"], c["rot"], c["axis_raw"], c["axis"], math.degrees(c["axis"]), c["ncor"], c["score"]))
axg = [c for c in cand.values() if disp.get(c["idx"], {}).get("erase") == "axis_gate"]
print("candidates erased by the 0.436 rad axis gate: %d ; their axis_raw range %s"
      % (len(axg), (min([c["axis_raw"] for c in axg]), max([c["axis_raw"] for c in axg])) if axg else None))

print()
print("== 6. angle families of the closest candidate per touching seam ==")
fam = Counter()
for f in [f for f in pairs if (int(f[1]), int(f[2])) in NINESET]:
    if f[4] == "-":
        continue
    r = cand[int(f[4])]["rot"]
    fam["<15" if r < 15 else "15-45" if r < 45 else "45-90" if r < 90 else "90-135" if r < 135 else "135-180"] += 1
    print("  %s-%s closest rot %.2f deg" % (f[1], f[2], r))
print("  family counts: %s" % dict(fam))
if a.clusters:
    print()
    print("== 7. 333's arm-1 clusters ==")
    for ln in open(a.clusters, errors="ignore"):
        print("  " + ln.rstrip())

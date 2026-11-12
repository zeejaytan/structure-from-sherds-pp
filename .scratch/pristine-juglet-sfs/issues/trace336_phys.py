#!/usr/bin/env python3
"""Ticket 336 -- class of each true seam under a PHYSICAL millimetre measure.

Companion to trace336.py (same log parser, same frames). The difference: the
translation criterion is point movement (RMS over the moving piece's breakline
points), not the norm of a residual transform's translation, which depends on
where the rotation origin sits (here ~1.5 m from the material).

Class rule: rotation <= 5 deg AND point movement <= 5 mm.
Precedence: A (surviving candidate) > B (candidate erased by a gate) >
            C (candidate merged away) > D (no candidate matches).

Usage: trace336_phys.py --log <binary log> --tree <dir holding 'Ground Truth'>
Writes <prefix>.seams9.phys.tsv and prints a matrix.
"""
import argparse, importlib.util, math, os

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location("t336", os.path.join(HERE, "trace336.py"))
m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)

TOL_DEG, TOL_MM = 5.0, 5.0


def load_pts(path):
    out, started = [], False
    for line in open(path):
        if line.startswith("DATA"):
            started = True
            continue
        if started:
            q = line.split()
            if len(q) >= 3:
                try:
                    out.append([float(q[0]), float(q[1]), float(q[2])])
                except ValueError:
                    pass
    return out


def ap(T, p):
    return [sum(T[r][c] * p[c] for c in range(3)) + T[r][3] for r in range(3)]


def rms_between(A, B, pts):
    s = 0.0
    for p in pts:
        a, b = ap(A, p), ap(B, p)
        s += sum((a[k] - b[k]) ** 2 for k in range(3))
    return math.sqrt(s / len(pts))


def tnorm(T):
    return math.sqrt(sum(T[r][3] ** 2 for r in range(3)))


def main():
    ap_ = argparse.ArgumentParser()
    ap_.add_argument("--log", required=True)
    ap_.add_argument("--tree", required=True)
    ap_.add_argument("--out", default=None)
    a = ap_.parse_args()
    out = a.out or (a.log.rsplit("/", 1)[-1].replace(".log", "") + ".phys")
    d = m.parse_log(a.log)
    fin = {i: v["M"] for i, v in d["final"].items()}
    axis = d["axis"]
    gt = {i: m.read_m4(os.path.join(a.tree, "Ground Truth", "Juglet_Piece_%d_T.txt" % i))
          for i in range(1, 10)}
    pts = {i: load_pts(os.path.join(a.tree, "Breaklines", "Juglet_Piece_%d_Breakline_0.pcd" % i))
           for i in range(1, 10)}
    merged_away = set(e["dropped"] for e in d["merge"] if e["site"] == "pair")
    rows = []
    for (u, v) in m.SEAMS9:
        if u not in fin or v not in fin:
            rows.append(dict(seam="%d-%d" % (u, v), cls="unplaced", idx=None, rot=None, rms=None,
                             tnorm=None, fg_rot=None, fg_rms=None, alt_idx=None, alt_rot=None,
                             alt_rms=None, note="no FINAL for piece(s) %s" % (
                                 ",".join(str(i) for i in (u, v) if i not in fin))))
            continue
        E = m.m4_mul(m.m4_inv(fin[u]), fin[v])
        G = m.m4_mul(m.m4_inv(gt[u]), gt[v])
        fg_rot = m.rot_deg(m.m4_mul(m.m4_inv(E), G))
        fg_rms = rms_between(E, G, pts[v])
        cands = []
        for idx, c in d["cand"].items():
            C = m.cand_orig(c, axis, u, v)
            if C is None:
                continue
            if idx in d["surv"]:
                st = "A"
            elif idx in d["drop"]:
                st = "B:" + d["drop"][idx]["reason"]
            elif idx in d["rdrop"]:
                st = "B:" + d["rdrop"][idx]["reason"]
            elif idx in merged_away:
                st = "C"
            else:
                st = "?"
            cands.append(dict(idx=idx, rot=m.rot_deg(m.m4_mul(m.m4_inv(E), C)),
                              rms=rms_between(E, C, pts[v]), tnorm=tnorm(m.m4_mul(m.m4_inv(E), C)),
                              st=st, gate=(d["drop"].get(idx) or d["rdrop"].get(idx) or {}).get("value")))
        hits = [x for x in cands if x["rot"] <= TOL_DEG and x["rms"] <= TOL_MM]
        alt = min(cands, key=lambda x: x["rot"]) if cands else None
        if hits:
            hits.sort(key=lambda x: {"A": 0, "C": 1}.get(x["st"][:1], 2) * 1000 + x["rms"])
            h = hits[0]
            rows.append(dict(seam="%d-%d" % (u, v), cls={"A": "A", "C": "C"}.get(h["st"][:1], "B"),
                             idx=h["idx"], rot=h["rot"], rms=h["rms"], tnorm=h["tnorm"],
                             fg_rot=fg_rot, fg_rms=fg_rms,
                             alt_idx=alt["idx"], alt_rot=alt["rot"], alt_rms=alt["rms"],
                             note=h["st"] + ("" if h["gate"] is None else " value=%.6g" % h["gate"])))
        else:
            rows.append(dict(seam="%d-%d" % (u, v), cls="D", idx=None, rot=None, rms=None, tnorm=None,
                             fg_rot=fg_rot, fg_rms=fg_rms,
                             alt_idx=(alt or {}).get("idx"), alt_rot=(alt or {}).get("rot"),
                             alt_rms=(alt or {}).get("rms"), note="no candidate within 5 deg / 5 mm"))
    with open(out + ".seams9.phys.tsv", "w") as f:
        f.write("seam\tclass\tidx\trot_deg\trms_mm\ttnorm_mm\tfinal_gt_rot\tfinal_gt_rms\t"
                "alt_idx\talt_rot\talt_rms\tnote\n")
        for r in rows:
            f.write("\t".join("" if r[k] is None else (("%.4f" % r[k]) if isinstance(r[k], float) else str(r[k]))
                              for k in ("seam", "cls", "idx", "rot", "rms", "tnorm", "fg_rot", "fg_rms",
                                        "alt_idx", "alt_rot", "alt_rms", "note")) + "\n")
    counts = {}
    for r in rows:
        counts[r["cls"]] = counts.get(r["cls"], 0) + 1
    print("%s  counts %s" % (os.path.basename(a.log), counts))
    for r in rows:
        print("  %-5s %-8s %-24s rot %s rms %s | final-vs-GT %s / %s | alt %s %s" % (
            r["seam"], r["cls"], ("idx %s" % r["idx"]) if r["idx"] is not None else "-",
            "-" if r["rot"] is None else "%.2f" % r["rot"], "-" if r["rms"] is None else "%.2f" % r["rms"],
            "-" if r["fg_rot"] is None else "%.2f" % r["fg_rot"],
            "-" if r["fg_rms"] is None else "%.2f" % r["fg_rms"],
            r["alt_idx"], "-" if r["alt_rot"] is None else "%.2f" % r["alt_rot"]))


if __name__ == "__main__":
    main()

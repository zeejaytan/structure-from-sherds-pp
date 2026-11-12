#!/usr/bin/env python3
"""Ticket 336 -- which stage wrote the final pose, per true seam (read-only analysis).

Inputs: one arm's binary log (SFS_STAGE_TRACE=1), the tree's "Ground Truth" directory.
Outputs: <prefix>.seams9.tsv, <prefix>.edges18.tsv, <prefix>.summary.json

Frames
  GT_i                       piece i original -> GT world (4x4 in Ground Truth/)
  F_i                        saved T_i (original -> assembly frame), log line FINAL
  E(u,v) = inv(F_u) F_v      pose of v in u's original frame; compare GTrel(u,v)=inv(GT_u) GT_v
  candidate trans_ A         aligned frame; maps moving x -> fixed y: A = inv(T_al_y) T_al_x
  C_orig(x->y) = inv(T_axis_y) A T_axis_x ; for seam u<v the target is E(u,v)
  NODE matrices              T_ in the aligned frame; aligned pose of the final is inv(T_axis_i) F_i
No numpy. Angle = acos((tr-1)/2) in degrees, translation in mm.
"""
import argparse, json, math, os, sys

SEAMS9 = [(1, 2), (1, 4), (1, 8), (1, 9), (2, 5), (2, 9), (3, 5), (6, 7), (7, 9)]
TOL_DEG = 5.0
TOL_MM = 5.0
PHASE_RANK = {"init": 0, "icp": 1, "refine": 2, "veto": 3}


def m4_from12(vals):
    M = [[float(vals[r * 4 + c]) for c in range(3)] + [float(vals[r * 4 + 3])] for r in range(3)]
    M.append([0.0, 0.0, 0.0, 1.0])
    return M


def m4_mul(A, B):
    return [[sum(A[r][k] * B[k][c] for k in range(4)) for c in range(4)] for r in range(4)]


def m4_inv(T):
    R = [[T[r][c] for c in range(3)] for r in range(3)]
    t = [T[r][3] for r in range(3)]
    Rt = [[R[c][r] for c in range(3)] for r in range(3)]
    ti = [-sum(Rt[r][k] * t[k] for k in range(3)) for r in range(3)]
    M = [[Rt[r][c] for c in range(3)] + [ti[r]] for r in range(3)]
    M.append([0.0, 0.0, 0.0, 1.0])
    return M


def rot_deg(M):
    tr = M[0][0] + M[1][1] + M[2][2]
    c = (tr - 1.0) * 0.5
    c = max(-1.0, min(1.0, c))
    return math.degrees(math.acos(c))


def trans_mm(M):
    return math.sqrt(M[0][3] ** 2 + M[1][3] ** 2 + M[2][3] ** 2)


def rel(A, B):
    return m4_mul(m4_inv(A), B)


def read_m4(path):
    rows = []
    with open(path, errors="replace") as f:
        for line in f:
            s = line.strip()
            if not s:
                continue
            rows.append([float(x) for x in s.replace(",", " ").split()])
            if len(rows) == 4:
                break
    if len(rows) != 4 or len(rows[0]) != 4:
        raise ValueError("not a 4x4 matrix: %s" % path)
    return rows


def read_graph(path):
    adj = {}
    with open(path, errors="replace") as f:
        for line in f:
            s = line.strip()
            if not s:
                continue
            vals = s.replace("\t", " ").split()
            if len(vals) < 2:
                continue
            try:
                vals = [int(float(v)) for v in vals]
            except ValueError:
                continue
            adj[vals[0]] = vals[1:]
    return adj


def parse_log(path):
    d = {"axis": {}, "cand": {}, "surv": {}, "rcand": [], "drop": {}, "rdrop": {},
         "merge": [], "node": [], "final": {}, "nodes_by_piece": {}, "outstate": None,
         "clusters": [], "prio": [], "veto": [], "steps": [], "keeps": [], "incr": [],
         "banners": []}
    with open(path, errors="replace") as f:
        for line in f:
            if "stage trace: " in line and "SFS" not in line[:10]:
                d["banners"].append(line.strip())
            if not line.startswith("[SFS-336]"):
                continue
            p = line.rstrip("\n").split("\t")
            try:
                k = p[1]
                if k == "AXIS":
                    d["axis"][int(p[2])] = m4_from12(p[3:15])
                elif k == "CAND":
                    i = int(p[2])
                    d["cand"][i] = dict(idx=i, y=int(p[3]), x=int(p[4]), score=float(p[5]),
                                        inlier=float(p[6]), axis_raw=float(p[7]), axis=float(p[8]),
                                        overlap=int(p[9]), area=float(p[10]), coverage=float(p[11]),
                                        M=m4_from12(p[12:24]))
                elif k == "SURV":
                    i = int(p[2])
                    d["surv"][i] = dict(idx=i, y=int(p[3]), x=int(p[4]), score=float(p[5]),
                                        axis=float(p[6]), M=m4_from12(p[7:19]))
                elif k == "RCAND":
                    d["rcand"].append(dict(step=int(p[2]), graph=int(p[3]), outside=int(p[4]),
                                           merge=int(p[5]), idx=int(p[6]), y=int(p[7]), x=int(p[8]),
                                           score=float(p[9]), axis_raw=float(p[10]), axis=float(p[11]),
                                           overlap=int(p[12]), area=float(p[13]),
                                           M=m4_from12(p[14:26])))
                elif k == "DROP":
                    d["drop"][int(p[3])] = dict(reason=p[2], idx=int(p[3]), y=int(p[4]), x=int(p[5]),
                                                value=float(p[6]))
                elif k == "RDROP":
                    d["rdrop"][int(p[5])] = dict(reason=p[2], step=int(p[3]), graph=int(p[4]),
                                                 idx=int(p[5]), y=int(p[6]), x=int(p[7]),
                                                 value=float(p[8]),
                                                 value2=float(p[9]) if len(p) > 9 else None)
                elif k == "MERGE":
                    d["merge"].append(dict(site=p[2], kept=int(p[3]), dropped=int(p[4]),
                                           y=int(p[5]), x=int(p[6]), r_kept=float(p[7]),
                                           r_dropped=float(p[8])))
                elif k == "NODE":
                    e = dict(step=int(p[2]), ext=int(p[3]), ord=int(p[4]), graph=int(p[5]),
                             phase=p[6], idx=int(p[7]), root=int(p[8]), M=m4_from12(p[9:21]))
                    d["node"].append(e)
                    d["nodes_by_piece"].setdefault(e["idx"], []).append(e)
                elif k == "INCR":
                    d["incr"].append(dict(step=int(p[2]), ext=int(p[3]), ord=int(p[4]),
                                          graph=int(p[5]), idx=int(p[6]), rot=float(p[7]),
                                          trans=float(p[8])))
                elif k == "VETO":
                    d["veto"].append(dict(step=int(p[2]), ext=int(p[3]), ord=int(p[4]),
                                          graph=int(p[5]), ok=int(p[6]), score=float(p[7]),
                                          merge=int(p[8])))
                elif k == "PRIO":
                    d["prio"].append(p[2:])
                elif k == "STEP":
                    d["steps"].append(p[2:])
                elif k == "KEEP":
                    d["keeps"].append(p[2:])
                elif k == "FINAL":
                    d["final"][int(p[2])] = dict(graph=int(p[3]), M=m4_from12(p[4:16]))
                elif k == "OUTSTATE":
                    d["outstate"] = dict(state=int(p[2]), score=float(p[3]), graphs=int(p[4]))
                elif k == "CLUSTER":
                    nodes = [int(v) for v in p[5].split(",")] if len(p) > 5 and p[5].strip() else []
                    d["clusters"].append(dict(graph=int(p[2]), root=int(p[3]), score=float(p[4]),
                                              nodes=nodes))
            except (ValueError, IndexError):
                continue
    return d


def cand_orig(c, axis, u, v):
    """C_orig for a candidate if it joins u and v, in the frame E(u,v) wants. None otherwise."""
    x, y = c["x"], c["y"]
    if x == y or not ({x, y} <= {u, v}):
        return None
    Tx, Ty = axis.get(x), axis.get(y)
    if Tx is None or Ty is None:
        return None
    C = m4_mul(m4_mul(m4_inv(Ty), c["M"]), Tx)   # C_orig(x->y)
    if x == v and y == u:
        return C                                  # v->u is E(u,v)
    return m4_inv(C)                              # u->v, invert


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--log", required=True)
    ap.add_argument("--tree", required=True, help="dir containing 'Ground Truth'")
    ap.add_argument("--out", default=None, help="output prefix")
    ap.add_argument("--nshard", type=int, default=9)
    a = ap.parse_args()

    gt_dir = os.path.join(a.tree, "Ground Truth")
    log = parse_log(a.log)
    prefix = a.out or os.path.join(os.path.dirname(os.path.abspath(a.log)), "trace336")
    notes = []

    if not log["axis"]:
        print("FATAL: no AXIS lines -- is SFS_STAGE_TRACE=1 and the binary the 336 build?")
        return 2
    if not log["final"]:
        print("FATAL: no FINAL lines")
        return 2

    gt = {}
    for i in sorted(log["final"]):
        p = os.path.join(gt_dir, "Juglet_Piece_%d_T.txt" % i)
        if not os.path.exists(p):
            p = os.path.join(gt_dir, "Piece_%d_T.txt" % i)
        if os.path.exists(p):
            gt[i] = read_m4(p)
    if len(gt) < 2:
        print("FATAL: no GT matrices under %s" % gt_dir)
        return 2

    axis = log["axis"]
    F = {i: v["M"] for i, v in log["final"].items()}
    gr = {i: v["graph"] for i, v in log["final"].items()}
    cl_of = {}
    for c in log["clusters"]:
        for n in c["nodes"]:
            cl_of[n] = c["graph"]

    # graph pose of the final: F = T_graph * T_axis (Trans::Input pre-multiplies), so
    # T_graph = F * inv(T_axis). This is what NODE lines print.
    Tal = {}
    for i in F:
        if i in axis:
            Tal[i] = m4_mul(F[i], m4_inv(axis[i]))

    # merge bookkeeping
    merged_away = {}
    for m in log["merge"]:
        merged_away[m["dropped"]] = m

    rows9, rows18 = [], []
    seams = SEAMS9 + [(x, y) for x in sorted(gt) for y in sorted(gt)
                      if x < y and (x, y) not in SEAMS9]
    for (u, v) in seams:
        row = dict(seam="%d-%d" % (u, v), class_="", deciding="", rot_err="", trans_err="",
                   matched_idx="", matched_rot="", matched_trans="", nearest_idx="", nearest_rot="",
                   gate="", gate_margin="", first_phase="", phase_trace="", clusters="", note="")
        if u not in F or v not in F:
            row["class_"] = "unplaced"
            miss = [str(i) for i in (u, v) if i not in F]
            row["note"] = "no FINAL for piece(s) " + ",".join(miss)
        elif u not in gt or v not in gt:
            row["class_"] = "no-gt"
            row["note"] = "no GT T file"
        else:
            E = rel(F[u], F[v])
            G = rel(gt[u], gt[v])
            d = rel(E, G)
            row["rot_err"] = "%.2f" % rot_deg(d)
            row["trans_err"] = "%.2f" % trans_mm(d)
            if u in cl_of and v in cl_of:
                row["clusters"] = "%d,%d" % (cl_of[u], cl_of[v])

            best = None
            nearest = None
            for idx, c in sorted(log["cand"].items()):
                C = cand_orig(c, axis, u, v)
                if C is None:
                    continue
                dd = rel(E, C)
                ang = rot_deg(dd)
                mm = trans_mm(dd)
                if nearest is None or ang < nearest[1]:
                    nearest = (idx, ang, mm)
                if best is None or (ang < best[1]):
                    best = (idx, ang, mm)
            if nearest is not None:
                row["nearest_idx"] = str(nearest[0])
                row["nearest_rot"] = "%.2f" % nearest[1]

            hit = None
            if best is not None and best[1] <= TOL_DEG and best[2] <= TOL_MM:
                hit = best
            if hit is not None:
                idx, ang, mm = hit
                row["matched_idx"] = str(idx)
                row["matched_rot"] = "%.2f" % ang
                row["matched_trans"] = "%.2f" % mm
                if idx in log["surv"]:
                    row["class_"] = "A"
                    row["deciding"] = "pair_survivor"
                elif idx in log["drop"]:
                    row["class_"] = "B"
                    dr = log["drop"][idx]
                    row["deciding"] = dr["reason"]
                    row["gate"] = dr["reason"]
                    val = dr["value"]
                    if dr["reason"].endswith("_axis"):
                        row["gate_margin"] = "%.3f deg over 24.985" % (math.degrees(val) - math.degrees(0.436))
                    elif dr["reason"] == "pair_basket":
                        row["gate_margin"] = "%.4f over 1.5" % (val - 1.5)
                    elif dr["reason"] == "pair_overlap":
                        row["gate_margin"] = "area %.1f mm2" % val
                elif idx in log["rdrop"]:
                    row["class_"] = "B"
                    dr = log["rdrop"][idx]
                    row["deciding"] = dr["reason"]
                    row["gate"] = dr["reason"]
                    if dr["reason"].endswith("_axis"):
                        row["gate_margin"] = "%.3f deg over 24.985" % (math.degrees(dr["value"]) - math.degrees(0.436))
                    elif dr["reason"] == "reg_score":
                        row["gate_margin"] = "%.4f over 10" % (dr["value"] - 10.0)
                    else:
                        row["gate_margin"] = "area %.1f mm2" % dr["value"]
                elif idx in merged_away:
                    row["class_"] = "C"
                    m = merged_away[idx]
                    row["deciding"] = "merge_" + m["site"]
                    row["gate"] = "merge_" + m["site"]
                    row["gate_margin"] = "rank %.5f vs kept %.5f" % (m["r_dropped"], m["r_kept"])
                else:
                    row["class_"] = "?"
                    row["note"] = "candidate matched but not in SURV/DROP/MERGE lists"

            if not row["class_"] or row["class_"] == "?":
                # Class D: first later boundary where the final relative appears.
                Er = rel(Tal[u], Tal[v]) if (u in Tal and v in Tal) else None
                cands = []
                for rc in log["rcand"]:
                    if rc["x"] == rc["y"] or not ({rc["x"], rc["y"]} <= {u, v}):
                        continue
                    if rc["x"] == rc["y"]:
                        continue
                    # aligned relative from the registration candidate
                    if rc["x"] == v and rc["y"] == u:
                        Eg = rc["M"]
                    elif rc["x"] == u and rc["y"] == v:
                        Eg = m4_inv(rc["M"])
                    else:
                        continue
                    d2 = rel(Eg, Er)
                    ang = rot_deg(d2)
                    if ang <= TOL_DEG and trans_mm(d2) <= TOL_MM:
                        cands.append((rc["step"], -1, "%d" % rc["graph"], "reg", ang,
                                      trans_mm(d2), rc["idx"]))
                groups = {}
                for e in log["node"]:
                    if e["idx"] not in (u, v):
                        continue
                    groups.setdefault((e["step"], e["ext"], e["ord"], e["graph"], e["phase"]), {})[e["idx"]] = e
                for key, g in sorted(groups.items()):
                    if u not in g or v not in g:
                        continue
                    Eg = rel(g[u]["M"], g[v]["M"])
                    d2 = rel(Eg, Er)
                    ang = rot_deg(d2)
                    if ang <= TOL_DEG and trans_mm(d2) <= TOL_MM:
                        cands.append((key[0], PHASE_RANK.get(key[4], 9), "%d" % key[3],
                                      key[4], ang, trans_mm(d2), None))
                cands.sort()
                if cands:
                    c0 = cands[0]
                    row["class_"] = "D"
                    row["first_phase"] = "%s@step%d" % (c0[3], c0[0])
                    row["deciding"] = ("registration" if c0[3] == "reg" else c0[3])
                    row["phase_trace"] = ";".join("%s@%d(%.2f%s)" % (c[3], c[0], c[4],
                                               ",g%s" % c[2] if c[2] else "") for c in cands[:6])
                    row["note"] = "first phase within 5 deg; %d matching phase dumps" % len(cands)
                else:
                    row["class_"] = "D"
                    row["deciding"] = "none"
                    row["note"] = "matches no phase dump within 5 deg (check FINAL graph vs trace)"

        if (u, v) in SEAMS9:
            rows9.append(row)
        else:
            rows18.append(row)

    cols = ["seam", "class_", "deciding", "rot_err", "trans_err", "matched_idx", "matched_rot",
            "matched_trans", "nearest_idx", "nearest_rot", "gate", "gate_margin", "first_phase",
            "phase_trace", "clusters", "note"]

    def wr(path, rows):
        with open(path, "w") as f:
            f.write("\t".join(cols) + "\n")
            for r in rows:
                f.write("\t".join(str(r[c]) for c in cols) + "\n")

    wr(prefix + ".seams9.tsv", rows9)
    wr(prefix + ".edges18.tsv", rows18)

    counts = {}
    dhist = {}
    for r in rows9:
        counts[r["class_"]] = counts.get(r["class_"], 0) + 1
        if r["class_"] == "D":
            k = r["deciding"]
            dhist[k] = dhist.get(k, 0) + 1

    summary = dict(log=a.log, tree=a.tree, banner=log["banners"][:2],
                   n_cand=len(log["cand"]), n_surv=len(log["surv"]), n_merge=len(log["merge"]),
                   n_rcand=len(log["rcand"]), n_drop=len(log["drop"]), n_rdrop=len(log["rdrop"]),
                   n_node=len(log["node"]), outstate=log["outstate"],
                   clusters=log["clusters"], classes9=counts, d_histogram=dhist,
                   unplaced=[i for i in range(1, a.nshard + 1) if i not in F])
    with open(prefix + ".summary.json", "w") as f:
        json.dump(summary, f, indent=1)

    print("seams9: " + ", ".join("%s=%d" % kv for kv in sorted(counts.items())))
    print("D stages: " + (", ".join("%s=%d" % kv for kv in sorted(dhist.items())) or "none"))
    print("clusters: " + " | ".join("g%d root%d nodes%s score%.4f" % (c["graph"], c["root"],
          c["nodes"], c["score"]) for c in log["clusters"]))
    print("wrote %s.seams9.tsv / .edges18.tsv / .summary.json" % prefix)
    return 0


if __name__ == "__main__":
    sys.exit(main())

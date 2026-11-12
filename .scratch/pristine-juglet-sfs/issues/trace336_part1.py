
#!/usr/bin/env python3
"""Trace analysis for ticket 336: which stage wrote the final pose, per true seam.

Reads one run's binary log (SFS_STAGE_TRACE=1), the tree's Ground Truth transforms, and
writes the nine-seam attribution table plus an all-GT-edges table. Read-only.

Conventions
  GT_i                     piece i original -> GT world (4x4, row-major in the file)
  F_i                      saved T_i (original -> assembly frame); log FINAL
  E(u,v) = inv(F_u) F_v    pose of v in u's original frame (frame independent)
  GTrel(u,v) = inv(GT_u) GT_v
  candidate A (trans_)     aligned frame: maps moving x -> fixed y (y's aligned frame)
  C_orig(x->y) = inv(T_axis_y) A T_axis_x   same relation in original frames
  seam (u<v) wants E(u,v): use C_orig(v->u) as-is, or inv(C_orig(u->v))
  aligned pose T^al_i = inv(T_axis_i) F_i (recovered), used for phase matching
"""
import argparse, json, os, sys

SEAMS9 = [(1,2),(1,4),(1,8),(1,9),(2,5),(2,9),(3,5),(6,7),(7,9)]
TOL_DEG = 5.0
TOL_MM  = 5.0
AXIS_GATE_RAD = 0.436
AXIS_GATE_DEG = AXIS_GATE_RAD * 180.0 / 3.141592653589793

def m4_from12(vals):
    M = [[float(vals[r*4+c]) for c in range(3)] + [float(vals[r*4+3])] for r in range(3)]
    M.append([0.0,0.0,0.0,1.0])
    return M

def m4_ident():
    return [[1.0,0,0,0],[0,1.0,0,0],[0,0,1.0,0],[0,0,0,1.0]]

def m4_mul(A,B):
    return [[sum(A[r][k]*B[k][c] for k in range(4)) for c in range(4)] for r in range(4)]

def m4_inv(T):
    R = [[T[r][c] for c in range(3)] for r in range(3)]
    t = [T[r][3] for r in range(3)]
    Rt = [[R[c][r] for c in range(3)] for r in range(3)]
    ti = [-sum(Rt[r][k]*t[k] for k in range(3)) for r in range(3)]
    M = [[Rt[r][c] for c in range(3)] + [ti[r]] for r in range(3)]
    M.append([0.0,0.0,0.0,1.0])
    return M

def rot_deg(M):
    tr = M[0][0] + M[1][1] + M[2][2]
    c = (tr - 1.0) * 0.5
    if c < -1.0: c = -1.0
    if c > 1.0: c = 1.0
    import math
    return math.degrees(math.acos(c))

def trans_mm(M):
    return (M[0][3]**2 + M[1][3]**2 + M[2][3]**2) ** 0.5

def rel(A,B):
    """pose of B in A's frame"""
    return m4_mul(m4_inv(A), B)

def read_m4(path):
    rows = []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line: continue
            rows.append([float(x) for x in line.split()])
    return rows[:4]

def parse_log(path):
    data = {"axis": {}, "cand": {}, "surv": {}, "rcand": [], "drop": {}, "rdrop": {},
            "merge": [], "node": [], "final": {}, "outstate": None, "clusters": [],
            "prio": [], "veto": [], "banner": None}
    with open(path, errors="replace") as f:
        for line in f:
            if "stage trace: " in line:
                data["banner"] = line.strip()
            if not line.startswith("[SFS-336]"):
                continue
            p = line.rstrip("\n").split("\t")
            kind = p[1] if len(p) > 1 else ""
            try:
                if kind == "AXIS":
                    data["axis"][int(p[2])] = m4_from12(p[3:15])
                elif kind == "CAND":
                    idx = int(p[2])
                    data["cand"][idx] = {"idx": idx, "y": int(p[3]), "x": int(p[4]),
                        "score": float(p[5]), "inlier": float(p[6]), "axis_raw": float(p[7]),
                        "axis": float(p[8]), "overlap": int(p[9]), "area": float(p[10]),
                        "coverage": float(p[11]), "M": m4_from12(p[12:24])}
                elif kind == "SURV":
                    idx = int(p[2])
                    data["surv"][idx] = {"idx": idx, "y": int(p[3]), "x": int(p[4]),
                        "score": float(p[5]), "axis": float(p[6]), "M": m4_from12(p[7:19])}
                elif kind == "RCAND":
                    data["rcand"].append({"step": int(p[2]), "graph": int(p[3]),
                        "outside": int(p[4]), "merge": int(p[5]), "idx": int(p[6]),
                        "y": int(p[7]), "x": int(p[8]), "score": float(p[9]),
                        "axis_raw": float(p[10]), "axis": float(p[11]),
                        "overlap": int(p[12]), "area": float(p[13]), "M": m4_from12(p[14:26])})
                elif kind == "DROP":
                    idx = int(p[3])
                    data["drop"][idx] = {"reason": p[2], "y": int(p[4]), "x": int(p[5]),
                                         "value": float(p[6])}
                elif kind == "RDROP":
                    idx = int(p[6])
                    data["rdrop"][idx] = {"reason": p[2], "step": int(p[3]), "graph": int(p[4]),
                        "y": int(p[5]), "x": int(p[6]), "value": float(p[8])}
                elif kind == "MERGE":
                    data["merge"].append({"site": p[2], "kept": int(p[3]), "dropped": int(p[4]),
                        "y": int(p[5]), "x": int(p[6]), "rank_kept": float(p[7]),
                        "rank_dropped": float(p[8])})
                elif kind == "NODE":
                    data["node"].append({"step": int(p[2]), "ext": int(p[3]), "ord": int(p[4]),
                        "graph": int(p[5]), "phase": p[6], "idx": int(p[7]),
                        "root": int(p[8]), "M": m4_from12(p[9:21])})
                elif kind == "VETO":
                    data["veto"].append({"step": int(p[2]), "ext": int(p[3]), "ord": int(p[4]),
                        "graph": int(p[5]), "pass": int(p[6]), "score": float(p[7]),
                        "merge": int(p[8])})
                elif kind == "PRIO":
                    data["prio"].append(p[2:])
                elif kind == "FINAL":
                    data["final"][int(p[2])] = m4_from12(p[4:16])
                elif kind == "OUTSTATE":
                    data["outstate"] = {"state": int(p[2]), "score": float(p[3]), "graphs": int(p[4])}
                elif kind == "CLUSTER":
                    data["clusters"].append({"graph": int(p[2]), "root": int(p[3]),
                        "score": float(p[4]), "nodes": p[5] if len(p) > 5 else ""})
            except (ValueError, IndexError):
                continue
    return data

def cand_to_orig(c, axis, u, v):
    """relation between seam pieces u<v in original frames, or None if not this seam.
    Returns (M, orientation) with M = C_orig(x->y) or its inverse as needed."""
    if not (c["x"] in (u,v) and c["y"] in (u,v)) or c["x"] == c["y"]:
        return None
    Tu = axis.get(c["x"]); Ty = axis.get(c["y"])
    if Tu is None or Ty is None:
        return None
    C = m4_mul(m4_mul(m4_inv(Ty), c["M"]), Tu)   # C_orig(x->y)
    if c["x"] == v and c["y"] == u:
        return C                                  # v->u == E(u,v)
    if c["x"] == u and c["y"] == v:
        return m4_inv(C)                          # invert to get v->u? no: C is u->v
    return None
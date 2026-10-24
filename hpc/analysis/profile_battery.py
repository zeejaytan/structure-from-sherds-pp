"""Offline plausibility-gate battery (ticket 10, T1/T2/T4'/T6).

Reads T0 dumps (profile validations with per-point piece+segment labels,
piece Ts, recorded verdicts). First replicates the live rule EXACTLY
(OLS fit per 7mm bin from zmin, max-absolute-deviation vs 7.0) and
requires 100% verdict reproduction — a replica that disagrees with the
recording is a broken replica, not a result. Then variants:

  T1 statistic: std-dev-of-orthogonal-errors rule (paper) on replica bins.
  T2 fit:      total-least-squares (orthogonal) refit per bin.
  T4' input:   drop non-wall segments (median spacing != ~1.9mm: raw
               ticket-14 appends), recompute. (T3 folds in: Breakline_0 IS
               the vote-interior wall, so "inner-only" == "wall-only";
               the pollution is appended content, owned by ticket 18.)
  T6 poses:    dumped piece Ts vs GT (rotation deg + translation mm).

Usage: python profile_battery.py <dumps-dir> <pot_a_gt-dir>
"""

import glob
import math
import os
import re
import sys

import numpy as np

BIN = 7.0
THRESH = 7.0
WALL_SPACING = 1.9
WALL_TOL = 0.6


def read_dump(path):
    meta = {}
    rows = []
    with open(path, errors="replace") as fh:
        for ln in fh:
            if ln.startswith("# verdict="):
                meta["verdict"] = ln.split("=")[1].split()[0]
            elif ln.startswith("# pieces:"):
                meta["pieces"] = [int(x) for x in ln.split(":")[1].split()]
            elif ln.startswith("# piece"):
                pass
            else:
                s = ln.split()
                if len(s) == 5:
                    rows.append([int(s[0]), int(s[1]),
                                 float(s[2]), float(s[3]), float(s[4])])
    return meta, np.array(rows, dtype=float)


def to_rz(xyz):
    r = np.hypot(xyz[:, 0], xyz[:, 1])
    return np.column_stack([r, xyz[:, 2]])  # (r, z)


def make_bins(rz):
    order = np.argsort(rz[:, 1], kind="stable")
    pts = rz[order]
    if not np.isfinite(pts).all():
        raise ValueError("non-finite profile points")
    zrange = pts[-1, 1] - pts[0, 1]
    zmin = pts[0, 1]
    nb = int(math.floor(zrange / BIN)) + 1
    if nb > 10000 or nb < 1:
        raise ValueError(f"absurd bin count {nb} for z-range {zrange}")
    out = []
    s = 0
    for b in range(nb):
        lo, hi = zmin + b * BIN, zmin + (b + 1) * BIN
        e = s
        while e < len(pts) and pts[e, 1] < hi:
            e += 1
        # live rule tests [start, end): last point of bin untested
        out.append((pts[s:e], s, e))
        s = e
    return out, order


def ols_fit(seg):
    z, r = seg[:, 1], seg[:, 0]
    A = np.column_stack([z, np.ones_like(z)])
    coef, *_ = np.linalg.lstsq(A, r, rcond=None)
    return coef  # r = m*z + c


def tls_fit(seg):
    z, r = seg[:, 1], seg[:, 0]
    C = np.column_stack([z - z.mean(), r - r.mean()])
    _, _, Vt = np.linalg.svd(C)
    nx, ny = Vt[-1]
    if len(seg) < 2:
        return None
    return nx, ny, seg[:, 0].mean(), seg[:, 1].mean()


def max_rule_dist(pt, coef):
    m, c = coef
    if abs(m) > 1:
        return abs(m * pt[1] - pt[0] + c) / math.hypot(m, 1)
    return abs(pt[0] - (m * pt[1] + c))


def replica_verdict(rz):
    """Live rule: OLS per bin, max abs deviation (last bin point untested)."""
    for seg, s, e in make_bins(rz)[0]:
        if len(seg) == 0:
            continue
        coef = ols_fit(seg)
        for pt in seg[:-1]:
            if max_rule_dist(pt, coef) > THRESH:
                return "FAILED"
    return "PASSED"


def stddev_verdict(rz):
    """Paper rule: std dev of orthogonal errors per bin (OLS fit kept)."""
    for seg, s, e in make_bins(rz)[0]:
        if len(seg) < 2:
            continue
        coef = ols_fit(seg)
        m, c = coef
        d = np.abs(m * seg[:, 1] - seg[:, 0] + c) / math.hypot(m, 1)
        if float(np.std(d)) > THRESH:
            return "FAILED"
    return "PASSED"


def tls_max_verdict(rz):
    for seg, s, e in make_bins(rz)[0]:
        if len(seg) < 2:
            continue
        f = tls_fit(seg)
        if f is None:
            continue
        nx, ny, mx, my = f
        for pt in seg[:-1]:
            d = abs(nx * (pt[0] - mx) + ny * (pt[1] - my))
            if d > THRESH:
                return "FAILED"
    return "PASSED"


def seg_spacing(rows, segid):
    sel = rows[rows[:, 1] == segid][:, 2:5]
    if len(sel) < 3:
        return 0.0
    return float(np.median(np.linalg.norm(np.diff(sel, axis=0), axis=1)))


def parse_Ts(path):
    """piece id -> (R 3x3, t 3)."""
    out = {}
    with open(path, errors="replace") as fh:
        for ln in fh:
            if not re.match(r"^# piece \d+ ", ln):
                continue
            s = ln.split()
            pid = int(s[2])
            iR = s.index("R=")
            it = next(k for k, tok in enumerate(s) if tok.startswith("t="))
            R = np.array([float(x) for x in s[iR + 1:iR + 10]]).reshape(3, 3)
            t = np.array([float(s[it].split("=", 1)[1])] +
                         [float(x) for x in s[it + 1:it + 3]])
            out[pid] = (R, t)
    return out


def gt_T(gtdir, piece):
    return np.loadtxt(os.path.join(
        gtdir, f"Pot_A_Piece_{piece}_T.txt")).reshape(4, 4)


def main():
    ddir, gtdir = sys.argv[1], sys.argv[2]
    paths = sorted(glob.glob(os.path.join(ddir, "*.tsv")))
    rep_ok = rep_bad = 0
    print(f"{'dump':12} {'pieces':14} {'recorded':8} "
          f"{'replica':8} {'stddev':8} {'tlsmax':8} {'wallonly':8}")
    for p in paths:
        try:
            meta, rows = read_dump(p)
        except Exception as e:
            print(f"{os.path.basename(p):12} UNREADABLE ({e})")
            continue
        try:
            xyz = rows[:, 2:5]
            rz = to_rz(xyz)
            rep = replica_verdict(rz)
        except Exception as e:
            print(f"{os.path.basename(p):12} {str(meta.get('pieces')):14} "
                  f"{meta.get('verdict', '?'):8} REPLICA-ERROR ({e})")
            continue
        ok = (rep == meta["verdict"])
        rep_ok += ok
        rep_bad += (not ok)
        stdv = stddev_verdict(rz)
        tlsv = tls_max_verdict(rz)
        # wall-only: keep segments with ~1.9mm median spacing
        keep = np.zeros(len(rows), dtype=bool)
        for sg in set(rows[:, 1].astype(int)) - {-1}:
            if abs(seg_spacing(rows, sg) - WALL_SPACING) <= WALL_TOL:
                keep |= rows[:, 1] == sg
        wall = replica_verdict(rz[keep]) if keep.sum() >= 2 else "SKIP"
        flag = "" if ok else "  <-- REPLICA MISMATCH"
        print(f"{os.path.basename(p):12} {str(meta['pieces']):14} "
              f"{meta['verdict']:8} {rep:8} {stdv:8} {tlsv:8} {wall:8}{flag}")
    print(f"\nreplica reproduction: {rep_ok}/{rep_ok + rep_bad}")
    # T6 poses: relative transform between pieces (frame-free) vs GT.
    print("\nT6 relative poses, dump vs GT (2-piece dumps):")
    for p in paths:
        meta, _ = read_dump(p)
        if len(meta["pieces"]) != 2:
            continue
        a, b = meta["pieces"]
        try:
            Ts = parse_Ts(p)
            Ra, ta = Ts[a]
            Rb, tb = Ts[b]
            Ga = gt_T(gtdir, a)
            Gb = gt_T(gtdir, b)
        except (KeyError, OSError):
            continue
        Rrel = Ra.T @ Rb
        Grel = Ga[:3, :3].T @ Gb[:3, :3]
        Rerr = Grel.T @ Rrel
        ang = math.degrees(math.acos(min(1.0, max(-1.0,
                                                 (np.trace(Rerr) - 1) / 2))))
        print(f"  {os.path.basename(p):12} pair {a}-{b}: "
              f"rel-rot err {ang:6.1f}deg")


if __name__ == "__main__":
    main()

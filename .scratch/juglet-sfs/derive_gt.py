"""Derive SFS++ Juglet inputs + GT from TORA's juglet_norm/gt hdf5 files.

Inputs : scan-pose meshes (juglet_norm.hdf5) at vessel scale (mm).
GT     : per-piece rigid input->assembled (Juglet_Piece_X_T.txt, SFS++ format)
         + adjacency (Juglet_simple_graph.txt, from GARF contact analysis,
         cross-checked vs TORA pairs file: 18 mates both).
Method : same two-stage fit as tora/scripts/build_juglet_ground_truth.py
         (exact order where kept, PCA+ICP where Blender shuffled it).

Vessel scale: 70.7098803591493 mm/file-unit (longest side = 65 mm).
"""
import json
import sys
from pathlib import Path

import h5py
import numpy as np
from scipy.spatial import cKDTree

IN_DIR = Path(r"C:\PR\structure-from-sherds-pp\artifacts\juglet_input")
SCRIPT_DIR = Path(__file__).resolve().parent
NORM = IN_DIR / "juglet_norm.hdf5"
GT = Path(r"C:\PR\tora\artifacts\juglet_gt.hdf5")
K = 70.7098803591493
OUT_MESH = IN_DIR / "Mesh"
OUT_GT = IN_DIR / "Ground Truth"


def umeyama(src, dst):
    """Similarity src->dst; returns s, R, t, mean_resid."""
    cs, cd = src.mean(0), dst.mean(0)
    AA, BB = src - cs, dst - cd
    H = AA.T @ BB
    U, S, Vt = np.linalg.svd(H)
    R = Vt.T @ np.diag([1.0, 1.0, np.sign(np.linalg.det(Vt.T @ U.T))]) @ U.T
    s = float((BB * (AA @ R)).sum() / max((AA ** 2).sum(), 1e-30))
    t = cd - s * (R @ cs)
    err = float(np.linalg.norm(s * (src @ R.T) + t - dst, axis=1).mean())
    return s, R, t, err


def align_similarity(src, dst):
    """PCA init + ICP loop with per-iteration similarity refit."""
    cs, cd = src.mean(0), dst.mean(0)
    A, B = src - cs, dst - cd
    Ua = np.linalg.svd(A.T @ A)[0]
    Ub = np.linalg.svd(B.T @ B)[0]
    # global scale guess from rms radii
    s0 = float(np.sqrt((B ** 2).sum() / max((A ** 2).sum(), 1e-30)))
    best = None
    for sx in (1.0, -1.0):
        for sy in (1.0, -1.0):
            D = np.diag([sx, sy, sx * sy])
            R = Ub @ D @ Ua.T
            if np.linalg.det(R) < 0:
                continue
            s, t = s0, cd - s0 * (R @ cs)
            cur = s * (src @ R.T) + t
            for _ in range(50):
                _, idx = cKDTree(dst).query(cur)
                s, R, t, _ = umeyama(src, dst[idx])
                cur = s * (src @ R.T) + t
            err = float(np.linalg.norm(cur - dst[cKDTree(dst).query(cur)[1]],
                                       axis=1).mean())
            if best is None or err < best[0]:
                best = (err, s, R, t)
    return best[1], best[2], best[3], best[0]


def solve_rigid(src, dst):
    cs, cd = src.mean(0), dst.mean(0)
    H = (src - cs).T @ (dst - cd)
    U, S, Vt = np.linalg.svd(H)
    R = Vt.T @ np.diag([1.0, 1.0, np.sign(np.linalg.det(Vt.T @ U.T))]) @ U.T
    return R, cd - R @ cs


def align_without_correspondence(src, dst):
    cs, cd = src.mean(0), dst.mean(0)
    A, B = src - cs, dst - cd
    Ua = np.linalg.svd(A.T @ A)[0]
    Ub = np.linalg.svd(B.T @ B)[0]
    best = None
    for sx in (1.0, -1.0):
        for sy in (1.0, -1.0):
            D = np.diag([sx, sy, sx * sy])
            R = Ub @ D @ Ua.T
            if np.linalg.det(R) < 0:
                continue
            t = cd - R @ cs
            cur = src @ R.T + t
            for _ in range(30):
                _, idx = cKDTree(dst).query(cur)
                dR, dt = solve_rigid(cur, dst[idx])
                cur = cur @ dR.T + dt
                R, t = dR @ R, dR @ t + dt
            err = float(np.linalg.norm(cur - dst[cKDTree(dst).query(cur)[1]],
                                       axis=1).mean())
            if best is None or err < best[0]:
                best = (err, R, t)
    return best[1], best[2], best[0]


def main():
    OUT_MESH.mkdir(parents=True, exist_ok=True)
    OUT_GT.mkdir(parents=True, exist_ok=True)
    hn = h5py.File(NORM, "r")
    hg = h5py.File(GT, "r")
    gn = hn["juglet_norm/Juglet-000/pieces"]
    gg = hg["juglet_gt/Juglet-000/pieces"]
    # Stage 1: the two files differ by ONE global similarity (same meshes,
    # normalized independently). Free-scale ICP degenerates (s -> 0), so fix
    # the scale from rms radii and fit rigidly only.
    sims = []
    for i in range(9):
        v_src = np.asarray(gn[str(i)]["vertices"][:])
        v_dst = np.asarray(gg[str(i)]["vertices"][:])
        assert len(v_src) == len(v_dst), (i, len(v_src), len(v_dst))
        s = float(np.sqrt(((v_dst - v_dst.mean(0)) ** 2).sum() / len(v_dst)
                          / max(((v_src - v_src.mean(0)) ** 2).sum()
                                / len(v_src), 1e-30)))
        sims.append(s)
    sims = np.array(sims)
    s_med = float(np.median(sims))
    spread = (sims.max() - sims.min()) / s_med * 100.0
    print(f"global scale norm->gt: {s_med:.5f} (spread {spread:.4f}%)")
    assert spread < 0.5, "pieces disagree on scale - wrong pairing?"
    # Rigid fit at fixed scale: exact order first (gt kept source order),
    # PCA+ICP fallback for shuffled pieces (tora build script §align).
    for i in range(9):
        v_src = np.asarray(gn[str(i)]["vertices"][:]) * s_med
        v_dst = np.asarray(gg[str(i)]["vertices"][:])
        R, t = solve_rigid(v_src, v_dst)
        err = float(np.linalg.norm(v_src @ R.T + t - v_dst, axis=1).mean())
        method = "exact-order"
        if err > 1e-6 * max(np.ptp(v_dst, axis=0).max(), 1e-9):
            R, t, err = align_without_correspondence(v_src, v_dst)
            method = "pca-icp"
            print(f"P{i + 1:02d}: n={len(v_src)} {method} "
                  f"mean_res={err:.2e}", flush=True)
        else:
            print(f"P{i + 1:02d}: n={len(v_src)} {method} "
                  f"mean_res={err:.2e}", flush=True)
        ang = float(np.degrees(np.arccos(np.clip((np.trace(R) - 1) / 2, -1, 1))))
        print(f"   rot={ang:6.1f}deg", flush=True)
    # Stage 2: inputs in mm at scan poses; GT_T rigid input_mm -> assembled_mm.
    for i in range(9):
        v_src = np.asarray(gn[str(i)]["vertices"][:]) * (K * s_med)
        v_dst = np.asarray(gg[str(i)]["vertices"][:]) * K
        R, t = solve_rigid(v_src, v_dst)
        err = float(np.linalg.norm(v_src @ R.T + t - v_dst, axis=1).mean())
        if err > 1e-6 * max(np.ptp(v_dst, axis=0).max(), 1e-9):
            R, t, _ = align_without_correspondence(v_src, v_dst)
            err = float(np.linalg.norm(v_src @ R.T + t - v_dst,
                                       axis=1).mean())
        ang = float(np.degrees(np.arccos(np.clip((np.trace(R) - 1) / 2, -1, 1))))
        # input mesh in scan pose, mm
        faces = np.asarray(gn[str(i)]["faces"][:]) + 1
        with open(OUT_MESH / f"Juglet_Piece_{i + 1:02d}_Mesh.obj", "w") as fh:
            fh.write(f"# Juglet Piece{i + 1:02d}, scan pose, mm "
                     f"(juglet_norm.hdf5 x{K * s_med:.6f})\n")
            for p in v_src:
                fh.write(f"v {p[0]:.4f} {p[1]:.4f} {p[2]:.4f}\n")
            for tri in faces:
                fh.write(f"f {tri[0]} {tri[1]} {tri[2]}\n")
        # GT transform: input_mm -> assembled_mm
        with open(OUT_GT / f"Juglet_Piece_{i + 1}_T.txt", "w") as fh:
            for r in range(3):
                fh.write(f"{R[r, 0]:.9f} {R[r, 1]:.9f} {R[r, 2]:.9f} "
                         f"{t[r]:.6f} \n")
            fh.write("0 0 0 1 \n")
        res_check = float(np.linalg.norm(v_src @ R.T + t - v_dst,
                                         axis=1).mean())
        print(f"P{i + 1:02d}: GT rigid rot={ang:6.1f}deg "
              f"mean_res={res_check:.4f}mm", flush=True)
    adj = json.load(open(IN_DIR / "adjacency_garf.json"))["adjacency_matrix"]
    with open(OUT_GT / "Juglet_simple_graph.txt", "w") as fh:
        for row in adj:
            fh.write("\t".join(map(str, row)) + "\t\n")
    print("wrote Mesh/ + Ground Truth/")


if __name__ == "__main__":
    sys.exit(main())

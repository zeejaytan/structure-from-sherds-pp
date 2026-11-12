#!/usr/bin/env python3
"""score58.py - ticket 58 scorer (measurements only; no recommendation).

Inputs:
  --result DIR   dir holding T_<i>to_<i>.txt (0-based piece index == GT piece i+1)
  --gt DIR       dir holding Juglet_Piece_<n>_T.txt (n = 1..9) and Juglet_simple_graph.txt
  --mesh DIR     optional: dir holding Juglet_Piece_<n>_Mesh.obj (own-place chamfer)
  --lanes DIR    optional: dir holding Juglet_Piece_<n>_Breakline_0.pcd (join residual)
  --out FILE     optional JSON output

Metrics
  A. binary-faithful reproduction of CountResult (class/data_structure.cpp:970-1040):
     rad_threshold 0.35 rad, t_threshold 50.0 mm, DIRECTED ordered pairs.
  B. per-piece raw error vs GT and after a single global rigid gauge (Kabsch).
  C. per-piece seating bar (docs/e2e-obj-to-reassembly.md:83-86): own-place chamfer
     < 7.07% of pot size AND median point distance < same bar.
  D. per-edge strict bar 5 deg / 10 mm and join residual (posed lanes).

Acceptance test (offline fixture, pristine_juglet_32994273):
  python3 score58.py --result artifacts/t58/prior/A --gt artifacts/t58/prior/B
  must print sherd 2/9 and edge k/2 = 0/18 with k = 1.
"""
import argparse, json, math, os, sys
import numpy as np

RAD = 0.35          # CountResult rad_threshold (radians)
TT = 50.0           # CountResult t_threshold (mm)
STRICT_DEG = 5.0    # ticket Task 2 strict bar
STRICT_MM = 10.0
POT_SIZE_MM = 65.0  # Juglet
SEAT_FRAC = 0.0707  # 7.07 % of pot size


def read_T(path):
    return np.loadtxt(path)


def read_graph(path):
    return np.loadtxt(path, dtype=int)


def rot_angle_deg(R):
    tr = np.clip((np.trace(R) - 1.0) / 2.0, -1.0, 1.0)
    return math.degrees(math.acos(tr))


def similar(GT_rel, E_rel, rad=RAD, tt=TT):
    D = GT_rel @ np.linalg.inv(E_rel)
    ang = rot_angle_deg(D[:3, :3])
    dt = float(np.linalg.norm(D[:3, 3]))
    return ang, dt, (ang < math.degrees(rad) and dt < tt)


def kabsch(P, Q):
    """R,t with R @ P_i + t ~ Q_i (P,Q: N x 3)."""
    Pc = P.mean(axis=0)
    Qc = Q.mean(axis=0)
    H = (P - Pc).T @ (Q - Qc)
    U, S, Vt = np.linalg.svd(H)
    d = np.sign(np.linalg.det(Vt.T @ U.T))
    D = np.diag([1.0, 1.0, d])
    R = Vt.T @ D @ U.T
    t = Qc - R @ Pc
    return R, t


def apply_G(G, M):
    T = np.eye(4)
    T[:3, :3] = G[0]
    T[:3, 3] = G[1]
    return T @ M


def read_obj_verts(path):
    V = []
    with open(path, 'r', errors='ignore') as fh:
        for line in fh:
            if line.startswith('v '):
                p = line.split()
                V.append((float(p[1]), float(p[2]), float(p[3])))
    return np.asarray(V, dtype=float)


def read_pcd_dots(path):
    """t55 Bl0 lane: 7 numbers per line after DATA ascii."""
    pts = []
    started = False
    with open(path, 'r', errors='ignore') as fh:
        for line in fh:
            if not started:
                if line.startswith('DATA'):
                    started = True
                continue
            p = line.split()
            if len(p) >= 3:
                pts.append((float(p[0]), float(p[1]), float(p[2])))
    return np.asarray(pts, dtype=float)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--result', required=True)
    ap.add_argument('--gt', required=True)
    ap.add_argument('--mesh')
    ap.add_argument('--lanes')
    ap.add_argument('--out')
    ap.add_argument('--label', default='run')
    a = ap.parse_args()

    graph = read_graph(os.path.join(a.gt, 'Juglet_simple_graph.txt'))
    ng = graph.shape[0]
    GT = []
    for i in range(ng):
        GT.append(read_T(os.path.join(a.gt, 'Juglet_Piece_%d_T.txt' % (i + 1))))

    res = {}
    have = []
    for i in range(ng):
        p = os.path.join(a.result, 'T_%dto_%d.txt' % (i, i))
        if os.path.exists(p):
            res[i] = read_T(p)
            have.append(i)

    out = {'label': a.label, 'gt_pieces': ng, 'result_dir': os.path.abspath(a.result),
           'pieces_with_T': [i + 1 for i in have], 'tables': {}}

    # ---- A. binary-faithful CountResult -------------------------------------
    k = 0
    total = 0
    pair_rows = []
    for i in range(ng):
        for j in range(ng):
            if not graph[i, j]:
                continue
            total += 1
            if i not in res or j not in res:
                continue
            GT_rel = np.linalg.inv(GT[i]) @ GT[j]
            E_rel = np.linalg.inv(res[i]) @ res[j]
            ang, dt, ok = similar(GT_rel, E_rel)
            if ok:
                k += 1
            pair_rows.append({'i': i + 1, 'j': j + 1, 'deg': round(ang, 4),
                              'mm': round(dt, 4), 'pass': bool(ok)})
    right = sorted({r['i'] for r in pair_rows if r['pass']} | {r['j'] for r in pair_rows if r['pass']})
    out['countresult'] = {
        'k_directed_pass': k, 'total_directed': 36,
        'k_sherd': len(right), 'right_sherd': right,
        'edge_print_pair': [k // 2, 18], 'pct': (k / 36.0 * 100.0) if total else 0.0,
    }

    # ---- B. per-piece GT error, raw and gauge-corrected ----------------------
    rows = []
    for i in have:
        dR = res[i][:3, :3] @ GT[i][:3, :3].T
        rows.append({'piece': i + 1,
                     'raw_deg': round(rot_angle_deg(dR), 2),
                     'raw_mm': round(float(np.linalg.norm(res[i][:3, 3] - GT[i][:3, 3])), 2)})
    P = np.array([res[i][:3, 3] for i in have])
    Q = np.array([GT[i][:3, 3] for i in have])
    if len(have) >= 3:
        Rg, tg = kabsch(P, Q)
        G = (Rg, tg)
        out['gauge'] = {'R': np.round(Rg, 6).tolist(), 't': np.round(tg, 4).tolist(),
                        'pieces_used': [i + 1 for i in have]}
    else:
        G = (np.eye(3), np.zeros(3))
        out['gauge'] = {'R': np.eye(3).tolist(), 't': [0, 0, 0], 'pieces_used': []}
    for r in rows:
        i = r['piece'] - 1
        Mg = apply_G(G, res[i])
        dR = Mg[:3, :3] @ GT[i][:3, :3].T
        r['gauge_deg'] = round(rot_angle_deg(dR), 2)
        r['gauge_mm'] = round(float(np.linalg.norm(Mg[:3, 3] - GT[i][:3, 3])), 2)
        r['strict_raw'] = bool(r['raw_deg'] < STRICT_DEG and r['raw_mm'] < STRICT_MM)
        r['strict_gauge'] = bool(r['gauge_deg'] < STRICT_DEG and r['gauge_mm'] < STRICT_MM)
    out['tables']['per_piece'] = rows

    # ---- C. own-place chamfer / median (needs meshes) ------------------------
    bar = POT_SIZE_MM * SEAT_FRAC
    out['seat_bar_mm'] = round(bar, 3)
    if a.mesh:
        try:
            from scipy.spatial import cKDTree
            seat = []
            for i in have:
                mp = os.path.join(a.mesh, 'Juglet_Piece_%d_Mesh.obj' % (i + 1))
                if not os.path.exists(mp):
                    mp = os.path.join(a.mesh, 'Juglet_Piece_%02d_Mesh.obj' % (i + 1))
                if not os.path.exists(mp):
                    continue
                V = read_obj_verts(mp)
                Est = (apply_G(G, res[i])[:3, :3] @ V.T).T + apply_G(G, res[i])[:3, 3]
                Hom = (GT[i][:3, :3] @ V.T).T + GT[i][:3, 3]
                te = cKDTree(Hom)
                th = cKDTree(Est)
                de, _ = te.query(Est)
                dh, _ = th.query(Hom)
                cham = float(0.5 * (de.mean() + dh.mean()))
                mede = float(np.median(de))
                medh = float(np.median(dh))
                seat.append({'piece': i + 1, 'verts': int(V.shape[0]),
                             'chamfer_mm': round(cham, 3),
                             'median_est2home_mm': round(mede, 3),
                             'median_home2est_mm': round(medh, 3),
                             'own_place': bool(cham < bar),
                             'right_way_round': bool(cham < bar and mede < bar and medh < bar)})
            out['tables']['seating'] = seat
            out['seated_count'] = sum(1 for s in seat if s['own_place'])
            out['right_way_count'] = sum(1 for s in seat if s['right_way_round'])
        except ImportError as e:
            out['tables']['seating'] = [{'error': 'scipy missing: %s' % e}]

    # ---- D. per-edge strict bar + join residual on posed lanes ---------------
    edge = []
    lanes = {}
    if a.lanes:
        for i in have:
            lp = os.path.join(a.lanes, 'Juglet_Piece_%d_Breakline_0.pcd' % (i + 1))
            if os.path.exists(lp):
                lanes[i] = read_pcd_dots(lp)
    for r in pair_rows:
        i = r['i'] - 1
        j = r['j'] - 1
        row = dict(r)
        row['strict'] = bool(r['deg'] < STRICT_DEG and r['mm'] < STRICT_MM)
        if i in lanes and j in lanes and i in res and j in res:
            try:
                from scipy.spatial import cKDTree
                Xi = (apply_G(G, res[i])[:3, :3] @ lanes[i].T).T + apply_G(G, res[i])[:3, 3]
                Xj = (apply_G(G, res[j])[:3, :3] @ lanes[j].T).T + apply_G(G, res[j])[:3, 3]
                tj = cKDTree(Xj)
                d, _ = tj.query(Xi)
                row['join_residual_median_mm'] = round(float(np.median(d)), 3)
                ti = cKDTree(Xi)
                d2, _ = ti.query(Xj)
                row['join_residual_median_mm_rev'] = round(float(np.median(d2)), 3)
                row['contact_pairs_lt2mm'] = int(((cKDTree(Xj).query(Xi)[0] < 2.0)).sum())
            except ImportError:
                pass
        edge.append(row)
    out['tables']['per_edge'] = edge

    txt = json.dumps(out, indent=1)
    if a.out:
        with open(a.out, 'w') as fh:
            fh.write(txt)
    print(txt)


if __name__ == '__main__':
    main()

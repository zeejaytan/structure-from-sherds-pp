"""Counterfactual for ticket 04: consistent vessel-axis directions.
Replaces each sherd's PotSAC direction with the PCA vessel axis expressed
in that sherd's scan frame. Positions untouched (isolates direction).
Backs originals to OUT_BACKUP; prints md5s for verification.
Usage: python3 cf_axes.py [--restore]
"""
import hashlib
import os
import sys

import numpy as np

DATA = "/data/gpfs/projects/punim2657/sfs_preprocessing"
BUNDLE = DATA + "/Juglet_Dataset_20260916/SfS_pp"
MESH = DATA + "/Dataset/Mesh/Juglet/Juglet_Piece_%d_Mesh.obj"
GT = DATA + "/Dataset/Juglet_GT/Juglet_Piece_%d_T.txt"
AXB = BUNDLE + "/Axes/Juglet_Piece_%d_Axis.xyz"
BACKUP = BUNDLE + "/_quarantine/cf_axes_orig"


def md5(p):
    with open(p, "rb") as f:
        return hashlib.md5(f.read()).hexdigest()


def load_obj_verts(path):
    v = []
    with open(path) as fh:
        for line in fh:
            if line.startswith("v "):
                v.append([float(x) for x in line.split()[1:4]])
    return np.array(v)


def load_T(path):
    return np.loadtxt(path).reshape(4, 4)


if "--restore" in sys.argv:
    for p in range(1, 10):
        src = os.path.join(BACKUP, "Juglet_Piece_%d_Axis.xyz" % p)
        dst = AXB % p
        with open(src, "rb") as fsrc, open(dst, "wb") as fdst:
            fdst.write(fsrc.read())
    print("restored:")
    for p in range(1, 10):
        print("  piece %d md5 %s" % (p, md5(AXB % p)))
    sys.exit(0)

print("original md5s:")
orig = {}
for p in range(1, 10):
    orig[p] = md5(AXB % p)
    print("  piece %d md5 %s" % (p, orig[p]))

# vessel axis = PC1 of GT-assembled meshes (geometry only, no PotSAC)
pts = []
Rs = {}
for p in range(1, 10):
    M = load_T(GT % p)
    Rs[p] = M[:3, :3]
    v = load_obj_verts(MESH % p)
    pts.append(v @ M[:3, :3].T + M[:3, 3])
P = np.vstack(pts)
C = P - P.mean(0)
_, _, Vt = np.linalg.svd(C, full_matrices=False)
v_vessel = Vt[0] / np.linalg.norm(Vt[0])
print("vessel axis (assembled frame):", np.round(v_vessel, 3))

os.makedirs(BACKUP, exist_ok=True)
for p in range(1, 10):
    with open(AXB % p, "rb") as f:
        raw = f.read()
    with open(os.path.join(BACKUP, "Juglet_Piece_%d_Axis.xyz" % p), "wb") as f:
        f.write(raw)
    a = np.fromstring(raw.decode(), sep=" ").reshape(-1, 6)[0]
    pos = a[:3]
    v_scan = Rs[p].T @ v_vessel
    v_scan /= np.linalg.norm(v_scan)
    with open(AXB % p, "w") as f:
        f.write("%.6f %.6f %.6f %.6f %.6f %.6f\n" % (tuple(pos) + tuple(v_scan)))
print("wrote consistent axes; verify:")
for p in range(1, 10):
    print("  piece %d md5 %s" % (p, md5(AXB % p)))

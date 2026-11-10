import numpy as np, os, sys, glob, re

DATA = r"C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330/data"

def read_T(path):
    lines = [l for l in open(path).read().splitlines() if l.strip()]
    M = np.eye(4)
    for i in range(len(lines)-1):
        v = lines[i].split()
        M[i,:] = [float(x) for x in v[:4]]
    return M

def rot_angle(R):
    c = (np.trace(R)-1.0)/2.0
    c = max(-1.0, min(1.0, c))
    return float(np.arccos(c))

def similar(A, B, rad=0.35, tmm=50.0):
    T = A @ np.linalg.inv(B)
    return rot_angle(T[:3,:3]) < rad and float(np.linalg.norm(T[:3,3])) < tmm

def load_run(d):
    S = {}
    for f in glob.glob(os.path.join(d, "T_*to_*.txt")):
        m = re.match(r"T_(\d+)to_(\d+)\.txt", os.path.basename(f))
        i, j = int(m.group(1)), int(m.group(2))
        assert i == j, f
        S[i] = read_T(f)
    return S

def load_gt():
    gdir = os.path.join(DATA, "x_gt", "Ground Truth")
    g = [f for f in glob.glob(os.path.join(gdir, "*.txt")) if "Piece" not in os.path.basename(f)]
    assert len(g) == 1, g
    G = np.loadtxt(g[0])
    GT = {}
    for f in glob.glob(os.path.join(gdir, "Pot_E_Piece_*_T.txt")):
        k = int(re.search(r"Piece_(\d+)_T", os.path.basename(f)).group(1))
        GT[k-1] = read_T(f)
    return G, GT

def count_result(S, GT, G, perm=None):
    n = G.shape[0]
    k = 0; total = 0; af = np.zeros((n,n), int)
    for i in range(n):
        for j in range(n):
            if G[i,j] == 0: continue
            total += 1
            GT_T = np.linalg.inv(GT[i]) @ GT[j]
            T = np.linalg.inv(S[i]) @ S[j]
            if similar(GT_T, T):
                k += 1; af[i,j] = 1; af[j,i] = 1
    ks = int(((af.sum(1) > 0)).sum())
    return ks, k//2, total//2, af

G, GT = load_gt()
print("GT graph sum(directed)=", int(G.sum()), "pieces=", G.shape[0])
for tag in ["t323_mor1_out", "t326_mor1_out", "t323_k3r1_out", "t326_k3r1_out"]:
    S = load_run(os.path.join(DATA, "x_"+tag))
    ks, ke, tot, af = count_result(S, GT, G)
    print(tag, "sherds", ks, "/", G.shape[0], " edges", ke, "/", tot)

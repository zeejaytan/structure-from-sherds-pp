
import numpy as np
def load_xyz(p):
    a = np.loadtxt(p)
    return a[:, :3] if a.ndim == 2 else a[:3].reshape(1,3)
def load_pcd(p):
    pts = []
    with open(p) as f:
        for ln in f:
            t = ln.split()
            if not t or t[0] == '#': continue
            try: v = [float(x) for x in t[:3]]
            except ValueError: continue
            pts.append(v)
    return np.array(pts)
def raxis(P, p0, d):
    d = d/np.linalg.norm(d); q = P - p0; t = q @ d
    return np.linalg.norm(q - np.outer(t, d), axis=1)
for i in (1, 2):
    a = np.loadtxt(f"Juglet_Piece_{i}_Axis.xyz").ravel(); p0, d = a[:3], a[3:6]
    s0 = load_xyz(f"Juglet_Piece_{i}_Surface_0.xyz"); s1 = load_xyz(f"Juglet_Piece_{i}_Surface_1.xyz")
    bl = load_pcd(f"Juglet_Piece_{i}_Breakline_0.pcd")
    r0, r1, rb = raxis(s0,p0,d), raxis(s1,p0,d), raxis(bl,p0,d)
    print(f"piece {i}: nS0={len(s0)} nS1={len(s1)} nBL={len(bl)} axispt={p0.round(1)} dir={d.round(2)}")
    print(f"   r(S0) med={np.median(r0):.2f}  r(S1) med={np.median(r1):.2f}  diff={np.median(r0)-np.median(r1):+.2f} mm")
    print(f"   r(BL) med={np.median(rb):.2f} range=[{rb.min():.2f},{rb.max():.2f}]")
    print(f"   centroid S0={s0.mean(0).round(1)} S1={s1.mean(0).round(1)} BL={bl.mean(0).round(1)}")

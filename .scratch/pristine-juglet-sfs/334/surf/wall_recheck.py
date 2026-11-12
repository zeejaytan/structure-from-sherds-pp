
import numpy as np
def load_xyz(p):
    a = np.loadtxt(p); return a[:, :3] if a.ndim == 2 else a[:3].reshape(1,3)
def load_pcd(p):
    pts=[]
    for ln in open(p):
        t=ln.split()
        if not t or t[0]=='#': continue
        try: v=[float(x) for x in t[:3]]
        except ValueError: continue
        pts.append(v)
    return np.array(pts)
def raxis(P,p0,d):
    d=d/np.linalg.norm(d); q=P-p0; t=q@d
    return np.linalg.norm(q-np.outer(t,d),axis=1)
print("piece  r(S0)  r(S1)  diff   r(BL)  wall_left(328)  sign328")
s328 = {1:"inner",2:"outer",3:"inner",4:"inner",5:"inner",6:"outer",7:"inner",8:"inner",9:"outer"}
for i in range(1,10):
    a=np.loadtxt(f"Juglet_Piece_{i}_Axis.xyz").ravel(); p0,d=a[:3],a[3:6]
    s0=load_xyz(f"Juglet_Piece_{i}_Surface_0.xyz"); s1=load_xyz(f"Juglet_Piece_{i}_Surface_1.xyz")
    r0,r1=raxis(s0,p0,d),raxis(s1,p0,d)
    bl=load_pcd(f"Juglet_Piece_{i}_Breakline_0.pcd"); rb=raxis(bl,p0,d)
    d0=float(np.median(r1)-np.median(r0))
    mine="outer" if d0<0 else "inner"
    print(f"{i:5d} {np.median(r0):6.2f} {np.median(r1):6.2f} {d0:+6.2f} {np.median(rb):6.2f}   {s328[i]:5s} -> 3x says {mine:5s} {'AGREE' if mine==s328[i] else 'DISAGREE'}")

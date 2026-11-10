import os, glob, json, math
import numpy as np
D = r"C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330"
def load_T(p):
    rows=[]
    for ln in open(p):
        s=ln.split()
        if len(s)>=4:
            try: rows.append([float(x) for x in s[:4]])
            except: pass
    M=np.eye(4)
    for i in range(min(3,len(rows))): M[i,:]=rows[i]
    return M
def load_Ts(d): return {int(os.path.basename(p)[2:].split("to")[0]): load_T(p) for p in glob.glob(os.path.join(d,"T_*to_*.txt"))}
GTD=os.path.join(D,"data","x_gt","Ground Truth")
gt={int(os.path.basename(p).split("_")[3])-1: load_T(p) for p in glob.glob(os.path.join(GTD,"*Piece_*_T.txt"))}
def rotdeg(M):
    c=max(-1,min(1,(np.trace(M[:3,:3])-1)/2)); return math.degrees(math.acos(c))
out={}
for lab in ["x_t323_mor1_out","x_t326_mor1_out","x_t326_k3r1_out"]:
    S=load_Ts(os.path.join(D,"data",lab))
    rows=[]
    for i in sorted(S):
        E=np.linalg.inv(gt[i]) @ S[i]
        rows.append((i+1, rotdeg(E), float(np.linalg.norm(E[:3,3]))))
    rows.sort(key=lambda r:-r[2])
    out[lab]=rows
    print(lab, "worst 8 (1-based piece, rot_deg, t_mm):")
    for r in rows[:8]: print("   P%02d  rot %7.2f  t %8.2f" % r)
    print("   over 10mm:", sum(1 for r in rows if r[2]>10))
json.dump(out, open(os.path.join(D,"abs_err.json"),"w"), indent=1)

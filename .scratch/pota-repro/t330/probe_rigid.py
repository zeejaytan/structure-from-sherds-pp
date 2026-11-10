import numpy as np, os, glob, re
DATA=r"C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330/data"
def read_T(p):
    ls=[l for l in open(p).read().splitlines() if l.strip()]; M=np.eye(4)
    for i in range(len(ls)-1): M[i,:]=[float(x) for x in ls[i].split()[:4]]
    return M
def rad(R):
    c=(np.trace(R)-1)/2; return float(np.arccos(max(-1,min(1,c))))
def load(d):
    S={}
    for f in glob.glob(os.path.join(d,"T_*to_*.txt")):
        m=re.match(r"T_(\d+)to_(\d+)\.txt",os.path.basename(f)); S[int(m.group(1))]=read_T(f)
    return S
gd=os.path.join(DATA,"x_gt","Ground Truth")
G=np.loadtxt([f for f in glob.glob(os.path.join(gd,"*.txt")) if "Piece" not in os.path.basename(f)][0])
gt=set((i+1,j+1) for i in range(31) for j in range(i+1,31) if G[i,j])
five=[3,24,25,26,31]
for tag in ("t323_mor1_out","t326_mor1_out"):
    S=load(os.path.join(DATA,"x_"+tag))
    print("==",tag)
    for a in five:
        hits=[]
        for b in range(1,32):
            if b==a: continue
            D=np.linalg.inv(S[a-1])@S[b-1]
            ang=np.degrees(rad(D[:3,:3])); tr=float(np.linalg.norm(D[:3,3]))
            if ang<5 and tr<20: hits.append("P%02d(%s,%.1fdeg,%.1fmm)"%(b,"GT" if (min(a,b),max(a,b)) in gt else "x",ang,tr))
        print("  P%02d <- %s"%(a, ", ".join(hits) if hits else "NONE"))

import numpy as np, os, glob, re
DATA=r"C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330/data"
def read_T(p):
    ls=[l for l in open(p).read().splitlines() if l.strip()]; M=np.eye(4)
    for i in range(len(ls)-1): M[i,:]=[float(x) for x in ls[i].split()[:4]]
    return M
def ra(R):
    c=max(-1.0,min(1.0,(np.trace(R)-1.0)/2.0)); return float(np.arccos(c))
def load_run(d):
    S={}
    for f in glob.glob(os.path.join(d,"T_*to_*.txt")):
        m=re.match(r"T_(\d+)to_(\d+)\.txt",os.path.basename(f)); S[int(m.group(1))]=read_T(f)
    return S
gd=os.path.join(DATA,"x_gt","Ground Truth")
G=np.loadtxt([f for f in glob.glob(os.path.join(gd,"*.txt")) if "Piece" not in os.path.basename(f)][0])
GT={}
for f in glob.glob(os.path.join(gd,"Pot_E_Piece_*_T.txt")):
    GT[int(re.search(r"Piece_(\d+)_T",os.path.basename(f)).group(1))-1]=read_T(f)
S=load_run(os.path.join(DATA,"x_t326_mor1_out"))
rows=[]
for i in range(G.shape[0]):
    for j in range(i+1,G.shape[0]):
        if G[i,j]==0: continue
        Gt=np.linalg.inv(GT[i])@GT[j]; T=np.linalg.inv(S[i])@S[j]
        A=Gt@np.linalg.inv(T)      # isSimilarTrans(GT_T, T)
        B=T@np.linalg.inv(Gt)
        rows.append((i+1,j+1,np.degrees(ra(A[:3,:3])),float(np.linalg.norm(A[:3,3])),np.degrees(ra(B[:3,:3])),float(np.linalg.norm(B[:3,3]))))
cntA=sum(1 for r in rows if r[2]<np.degrees(0.35) and r[3]<50.0)
cntB=sum(1 for r in rows if r[4]<np.degrees(0.35) and r[5]<50.0)
print("countA(gt@invT)",cntA,"countB(T@invg)",cntB)
for r in rows:
    okA=r[2]<np.degrees(0.35) and r[3]<50.0; okB=r[4]<np.degrees(0.35) and r[5]<50.0
    if okA!=okB or r[3]>44 or r[5]>44:
        print("P%02d P%02d  A %.3fdeg %.3fmm  B %.3fdeg %.3fmm  A=%s B=%s"%(r[0],r[1],r[2],r[3],r[4],r[5],okA,okB))

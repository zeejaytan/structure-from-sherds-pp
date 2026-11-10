import numpy as np, os, glob, re, json
DATA = r"C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330/data"
def read_T(path):
    lines=[l for l in open(path).read().splitlines() if l.strip()]
    M=np.eye(4)
    for i in range(len(lines)-1):
        M[i,:]=[float(x) for x in lines[i].split()[:4]]
    return M
def rot_deg(R):
    c=max(-1.0,min(1.0,(np.trace(R)-1.0)/2.0)); return float(np.degrees(np.arccos(c)))
def load_run(d):
    S={}
    for f in glob.glob(os.path.join(d,"T_*to_*.txt")):
        m=re.match(r"T_(\d+)to_(\d+)\.txt",os.path.basename(f)); i=int(m.group(1)); S[i]=read_T(f)
    return S
gdir=os.path.join(DATA,"x_gt","Ground Truth")
G=np.loadtxt([f for f in glob.glob(os.path.join(gdir,"*.txt")) if "Piece" not in os.path.basename(f)][0])
GT={}
for f in glob.glob(os.path.join(gdir,"Pot_E_Piece_*_T.txt")):
    k=int(re.search(r"Piece_(\d+)_T",os.path.basename(f)).group(1)); GT[k-1]=read_T(f)
rows=[]
for tag in ["t323_mor1_out","t326_mor1_out"]:
    S=load_run(os.path.join(DATA,"x_"+tag))
    for i in range(G.shape[0]):
        for j in range(i+1,G.shape[0]):
            if G[i,j]==0: continue
            Tg=np.linalg.inv(GT[i])@GT[j]; T=np.linalg.inv(S[i])@S[j]
            D=T@np.linalg.inv(Tg)
            rows.append((tag,i+1,j+1,rot_deg(D[:3,:3]),float(np.linalg.norm(D[:3,3])),bool(rot_deg(D[:3,:3])<np.degrees(0.35) and np.linalg.norm(D[:3,3])<50.0)))
json.dump(rows,open(r"C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330/edge_err.json","w"))
bad=[r for r in rows if r[0]=="t326_mor1_out" and not r[5]]
print("t326 MOR1 failed",len(bad))
for r in sorted(bad,key=lambda x:-x[3]):
    ok=[q for q in rows if q[0]=="t323_mor1_out" and q[1]==r[1] and q[2]==r[2]][0]
    print("P%02d P%02d t326 %6.2fdeg %7.2fmm   t323 %6.2fdeg %7.2fmm" % (r[1],r[2],r[3],r[4],ok[3],ok[4]))

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
def load_Ts(d):
    return {int(os.path.basename(p)[2:].split("to")[0]): load_T(p) for p in glob.glob(os.path.join(d,"T_*to_*.txt"))}
def rel(A,B): return np.linalg.inv(A) @ B
def rotdeg(M):
    c=max(-1,min(1,(np.trace(M[:3,:3])-1)/2)); return math.degrees(math.acos(c))
def trn(M): return float(np.linalg.norm(M[:3,3]))
C=json.load(open(os.path.join(D,"contacts.json")))
GTD=os.path.join(D,"data","x_gt","Ground Truth")
G=np.array([[int(x) for x in l.split()] for l in open(os.path.join(GTD,"Pot_E_simple_graph.txt")) if l.strip()])
gedges=set((min(i,j),max(i,j)) for i in range(31) for j in range(31) if G[i,j])
for lab in ["x_t323_mor1_out","x_t326_mor1_out"]:
    A=np.array(C[lab],dtype=float); np.fill_diagonal(A,np.nan)
    S=load_Ts(os.path.join(D,"data",lab))
    rows=[]
    for i in range(31):
        for j in range(i+1,31):
            if A[i,j]<=1.0:
                M=rel(S[i],S[j])
                rows.append((i+1,j+1,(i,j) in gedges,A[i,j],rotdeg(M),trn(M)))
    print("\n",lab,"contacts<=1mm",len(rows))
    p20=[r for r in rows if r[4]<10.03 and r[5]<20.0]   # isSimilar rad 0.175 rad = 10.03 deg
    p3093=[r for r in rows if r[4]<10.03 and r[5]<30.92]
    print("  pass merge gate pristine(10.03deg,20mm):",len(p20)," patched(10.03deg,30.92mm):",len(p3093))
    extra=[r for r in p3093 if r not in p20]
    print("  extra under patched:",[(r[0],r[1],r[2],round(r[5],2)) for r in extra])
    ng=[r for r in rows if not r[2]]
    print("  non-GT contacts:",[(r[0],r[1],round(r[3],2),round(r[4],1),round(r[5],1)) for r in ng])
    print("  GT contacts failing merge gate(30.92):",[(r[0],r[1],round(r[4],1),round(r[5],1)) for r in rows if r[2] and not (r[4]<10.03 and r[5]<30.92)])

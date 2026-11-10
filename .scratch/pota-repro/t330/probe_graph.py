import os, glob, json, math, collections
import numpy as np
D = r"C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330"
C=json.load(open(os.path.join(D,"contacts.json")))
GTD=os.path.join(D,"data","x_gt","Ground Truth")
G=np.array([[int(x) for x in l.split()] for l in open(os.path.join(GTD,"Pot_E_simple_graph.txt")) if l.strip()])
gedges=set((min(i,j),max(i,j)) for i in range(31) for j in range(31) if G[i,j])
def comps(A,thr):
    n=A.shape[0]; seen=set(); out=[]
    for s in range(n):
        if s in seen: continue
        st=[s]; c=[]; seen.add(s)
        while st:
            u=st.pop(); c.append(u)
            for w in range(n):
                if w not in seen and A[u,w]<=thr: seen.add(w); st.append(w)
        out.append(sorted(c))
    return out
out={}
for lab in ["x_t323_mor1_out","x_t326_mor1_out"]:
    A=np.array(C[lab],dtype=float)
    np.fill_diagonal(A,np.nan)
    thr=1.0
    touch=[(i,j) for i in range(31) for j in range(i+1,31) if A[i,j]<=thr]
    cs=comps(A,thr)
    print("\n==",lab,"touching pairs(<=1mm):",len(touch),"components:",len(cs),[ [x+1 for x in c] for c in cs if len(c)<=8])
    nonGT=[(i,j,round(float(A[i,j]),2)) for (i,j) in touch if (i,j) not in gedges]
    print("   non-GT touching:",len(nonGT), nonGT)
    miss=[e for e in gedges if not (A[e]<thr)]
    print("   GT edges with no <=1mm contact:",len(miss),[(i+1,j+1) for i,j in sorted(miss)])
    out[lab]={"touch":[[i,j,A[i,j]] for i,j in touch]}
json.dump(out,open(os.path.join(D,"touch.json"),"w"))

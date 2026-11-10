import os, glob, math, json
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
    out={}
    for p in glob.glob(os.path.join(d,"T_*to_*.txt")):
        i=int(os.path.basename(p)[2:].split("to")[0]); out[i]=load_T(p)
    return out
def rel(A,B): return np.linalg.inv(A) @ B
def rott(M):
    c=max(-1,min(1,(np.trace(M[:3,:3])-1)/2)); return math.acos(c)
def trn(M): return float(np.linalg.norm(M[:3,3]))
GTD=os.path.join(D,"data","x_gt","Ground Truth")
gt={}
for p in glob.glob(os.path.join(GTD,"*Piece_*_T.txt")):
    gt[int(os.path.basename(p).split("_")[3])-1]=load_T(p)
G=np.array([[int(x) for x in l.split()] for l in open(os.path.join(GTD,"Pot_E_simple_graph.txt")) if l.strip()])
edges=[(i,j) for i in range(G.shape[0]) for j in range(G.shape[1]) if G[i,j]]
print("directed", len(edges), "undirected", len(edges)//2)
def score(st):
    ok=[]; bad=[]
    for (i,j) in edges:
        g=rel(gt[i],gt[j]); t=rel(st[i],st[j])
        M=g@np.linalg.inv(t)
        (ok if (rott(M)<0.35 and trn(M)<50.0) else bad).append((i,j))
    sherd_ok=set()
    for a,b in ok: sherd_ok.add(a); sherd_ok.add(b)
    return ok,bad,sherd_ok
res={}
for lab in ["x_t323_mor1_out","x_t326_mor1_out","x_t323_k3r1_out","x_t326_k3r1_out"]:
    st=load_Ts(os.path.join(D,"data",lab))
    ok,bad,sh=score(st)
    res[lab]={"sherd":len(sh),"edges":len(ok),"bad":sorted(set(tuple(sorted(b)) for b in bad))}
    print(lab,"sherds",len(sh),"/",len(gt),"edges",len(ok),"/",len(edges)//2,"bad undirected",len(res[lab]["bad"]))
    print("   bad:", res[lab]["bad"])
json.dump(res, open(os.path.join(D,"state_verdicts.json"),"w"), indent=1)

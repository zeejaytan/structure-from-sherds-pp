import os, json, collections
import numpy as np
D = r"C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330"
C=json.load(open(os.path.join(D,"contacts.json")))
V=json.load(open(os.path.join(D,"state_verdicts.json")))
GTD=os.path.join(D,"data","x_gt","Ground Truth")
G=np.array([[int(x) for x in l.split()] for l in open(os.path.join(GTD,"Pot_E_simple_graph.txt")) if l.strip()])
gedges=set((min(i,j),max(i,j)) for i in range(31) for j in range(31) if G[i,j])
A=np.array(C["x_t326_mor1_out"],dtype=float); np.fill_diagonal(A,np.nan)
thr=1.0
def bfs(s,t):
    prev={s:None}; q=collections.deque([s])
    while q:
        u=q.popleft()
        if u==t: break
        for w in range(31):
            if w not in prev and A[u,w]<=thr: prev[w]=u; q.append(w)
    if t not in prev: return None
    p=[t]
    while prev[p[-1]] is not None: p.append(prev[p[-1]])
    return p[::-1]
bad=[tuple(sorted(e)) for e in V["x_t326_mor1_out"]["bad"]]
print("failed GT joins:", len(bad))
for (i,j) in sorted(bad):
    p=bfs(i,j)
    if p is None:
        print("  P%02d-P%02d : no path (different contact components)" % (i+1,j+1))
    else:
        pe=[(min(a,b)+1,max(a,b)+1) for a,b in zip(p,p[1:])]
        nongt=[e for e in pe if (e[0]-1,e[1]-1) not in gedges]
        print("  P%02d-P%02d : path %s  non-GT links %s" % (i+1,j+1,"-".join("P%02d"%x for x in [q+1 for q in p]), nongt))

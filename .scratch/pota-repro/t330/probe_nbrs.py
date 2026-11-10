import os, json
import numpy as np
D = r"C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330"
C=json.load(open(os.path.join(D,"contacts.json")))
V=json.load(open(os.path.join(D,"state_verdicts.json")))
GTD=os.path.join(D,"data","x_gt","Ground Truth")
G=np.array([[int(x) for x in l.split()] for l in open(os.path.join(GTD,"Pot_E_simple_graph.txt")) if l.strip()])
gedges=set((min(i,j),max(i,j)) for i in range(31) for j in range(31) if G[i,j])
A=np.array(C["x_t326_mor1_out"],dtype=float); np.fill_diagonal(A,np.nan)
bad=sorted(set(tuple(sorted(e)) for e in V["x_t326_mor1_out"]["bad"]))
def nbrs(i,thr=1.0): return sorted([(j+1,round(float(A[i,j]),2), (min(i,j),max(i,j)) in gedges) for j in range(31) if A[i,j]<=thr])
print("final-state neighbourhoods (piece, gap_mm, is_GT) for the misplaced sherds and their partners")
for p in [2,22,23,24,25,29,4,12,13,16,17,19,27]:
    print("  P%02d  gt_nbrs=%s" % (p+1, sorted([q+1 for q in range(31) if (min(p,q),max(p,q)) in gedges])))
    print("        touching(<=1mm)=%s" % (nbrs(p),))

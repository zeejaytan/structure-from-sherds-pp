
import json, itertools
base = "C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330/"
d = json.load(open(base+"contacts.json"))
gtp = "C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330/data/x_gt/Ground Truth/Pot_E_simple_graph.txt"
G = []
for line in open(gtp):
    s=line.split()
    if not s: continue
    if all(x.replace('.','',1).lstrip('-').isdigit() for x in s):
        G.append([float(x) for x in s])
print("GT dims", len(G), len(G[0]))
gt = set()
for i in range(len(G)):
    for j in range(len(G)):
        if i!=j and G[i][j]: gt.add((min(i+1,j+1),max(i+1,j+1)))
five = {3,24,25,26,31}
for lab in ("x_t323_mor1_out","x_t326_mor1_out"):
    M = d[lab]
    print("==", lab)
    for i,j in sorted(itertools.combinations(range(31),2)):
        v = M[i][j]
        if v is None or v!=v: continue
        if v <= 1.0 and (i+1 in five or j+1 in five):
            print("  P%02d P%02d d=%.2f gt=%s" % (i+1,j+1,v,(i+1,j+1) in gt))

import json, itertools
base=r"C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330/"
d=json.load(open(base+"contacts.json"))
G=[]
gd=r"C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330/data/x_gt/Ground Truth/Pot_E_simple_graph.txt"
for line in open(gd):
    s=line.split()
    if s and all(x.replace('.','',1).lstrip('-').isdigit() for x in s): G.append([float(x) for x in s])
gt=set((i+1,j+1) for i in range(31) for j in range(i+1,31) if G[i][j])
def touch(lab,cut=1.0):
    M=d[lab]; out={}
    for i,j in itertools.combinations(range(31),2):
        v=M[i][j]
        if v is None or v!=v: continue
        if v<=cut: out[(i+1,j+1)]=v
    return out
a=touch("x_t323_mor1_out"); b=touch("x_t326_mor1_out")
print("t323 touching",len(a),"t326 touching",len(b))
new=sorted(set(b)-set(a)); gone=sorted(set(a)-set(b))
print("\nNEW in t326 (not touching in t323): %d"%len(new))
for p in new: print("  P%02d-P%02d d=%.2f gt=%s"%(p[0],p[1],b[p],p in gt))
print("\nGONE in t326 (touching only in t323): %d"%len(gone))
for p in gone: print("  P%02d-P%02d d=%.2f gt=%s"%(p[0],p[1],a[p],p in gt))
print("\nstill touching both:",len(set(a)&set(b)),"of which non-GT:",[p for p in sorted(set(a)&set(b)) if p not in gt])

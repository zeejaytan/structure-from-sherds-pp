
import os, collections
D = r"C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330/lead_audit"
def load(p):
    rows={}; surv=set()
    for line in open(p):
        f=line.rstrip("\n").split("\t")
        if len(f)<2 or not f[1].strip().isdigit(): continue
        if f[0]=='CAND': rows[int(f[1])]=f
        elif f[0]=='SURVIVOR': surv.add(int(f[1]))
    return rows,surv
c,s60=load(os.path.join(D,'audit_MOR1_S60.txt'))
_,s92=load(os.path.join(D,'audit_MOR1_S9277.txt'))
def pair(i): f=c[i]; return (min(int(f[2]),int(f[3])),max(int(f[2]),int(f[3])))
failed=[(3,5),(3,17),(3,24),(3,25),(3,26),(5,25),(13,25),(14,26),(23,24),(24,31),(26,30),(28,31),(14,30),(17,18),(18,23),(20,23)]
byp=collections.defaultdict(list)
for i in c: byp[pair(i)].append(i)
hi=0
for p in failed:
    ks=byp.get(p,[]); gtk=[i for i in ks if c[i][4]=='1']
    sb=[i for i in gtk if i in s60]; s9=[i for i in gtk if i in s92]
    both=[i for i in gtk if i in s60 and i in s92]
    if both: hi+=1
    print("%-8s cands=%d gt=%d | gt-surv S60=%s S92=%s both=%s"%(str(p),len(ks),len(gtk),sb,s9,both))
print("failed joins with a GT candidate surviving in BOTH regimes:",hi,"/16")

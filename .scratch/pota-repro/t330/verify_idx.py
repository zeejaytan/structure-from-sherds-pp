
import os
D=r"C:\PR\worktrees\structure-from-sherds-pp\330\.scratch\pota-repro\t330\out"
def parse(p):
    cand={};merges=[]
    for ln in open(p,errors="ignore"):
        s=ln.rstrip("\n").split("\t")
        if not s: continue
        if s[0]=="CAND":
            if s[1]=="idx": continue
            cand[int(s[1])]=(int(s[2]),int(s[3]),int(s[4]))
        elif s[0]=="MERGE":
            if s[1]=="kept_idx": continue
            merges.append(s)
    return cand,merges
for lane in ["MOR1","K3R1"]:
    a,ma=parse(os.path.join(D,"audit_%s_S60.txt"%lane))
    b,mb=parse(os.path.join(D,"audit_%s_S9277.txt"%lane))
    diff=[i for i in a if i in b and a[i]!=b[i]]
    print(lane,"cands",len(a),len(b),"mismatched idx:",len(diff),"missing in one:",len(set(a)^set(b)))
    print("  merges S60=%d S9277=%d"%(len(ma),len(mb)))
    for tag,m in (("S60",ma),("S9277",mb)):
        wide=[x for x in m if 20.0 < float(x[8]) <= 30.93]
        print("  %s merges with 20<t<=30.93: %d %s"%(tag,len(wide),[(x[3],x[4],round(float(x[8]),1)) for x in wide]))

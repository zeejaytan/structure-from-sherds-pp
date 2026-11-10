
import json,os
D=r"C:\PR\worktrees\structure-from-sherds-pp\330\.scratch\pota-repro\t330"
OUT=os.path.join(D,"out")
def parse(p):
    cand={};drop={};surv={}
    for ln in open(p,errors="ignore"):
        s=ln.rstrip("\n").split("\t")
        if not s: continue
        k=s[0]
        if k=="CAND":
            if s[1]=="idx": continue
            i=int(s[1])
            cand[i]=dict(i=i,y=int(s[2]),x=int(s[3]),gt=int(s[4]),score=float(s[6]),inlier=int(s[7]),
                area=float(s[9]),axis=float(s[10]),rot=float(s[11]),tn=float(s[12]),gap=float(s[14]))
        elif k=="DROP":
            if s[1]=="idx": continue
            drop[int(s[1])]=s[4]
        elif k=="SURVIVOR":
            if s[1]=="idx": continue
            surv[int(s[1])]=1
    return cand,drop,surv
ct=json.load(open(os.path.join(D,"contacts.json")))
def cd(a,b):
    for key in ("%d-%d"%(a,b),"%d-%d"%(b,a)):
        if key in ct: return ct[key]
    return None
ca,da,sa=parse(os.path.join(OUT,"audit_MOR1_S60.txt"))
cb,db,sb=parse(os.path.join(OUT,"audit_MOR1_S9277.txt"))
bad=[(2,4),(2,16),(2,23),(2,24),(2,25),(4,24),(12,24),(13,25),(13,29),(16,17),(17,22),(19,22),(22,23),(23,30),(25,29),(27,30)]
def key(c): return (c["y"],c["x"])
print("join\tGT?\t#cand\t#GTok_S60\t#GTok_S9277\tGTcand_surv_both\tbest idx\trot\ttn\tcontact_t326")
for (a,b) in bad:
    p=(a+1,b+1)
    L=[c for c in ca.values() if key(c)==p]
    L=sorted(L,key=lambda c:c["i"])
    def gtok(c): return c["gt"]==1 and c["rot"]<20.05 and c["tn"]<50.0
    g60=[c for c in L if gtok(c) and c["i"] in sa]
    g92=[c for c in L if gtok(c) and c["i"] in sb]
    gt=1 if any(c["gt"]==1 for c in L) else 0
    best=None
    allg=[c for c in L if gtok(c)]
    bst=min(allg,key=lambda c:c["rot"]**2+c["tn"]**2) if allg else None
    print("%02d-%02d\t%d\t%d\t%d\t%d\t%s\t%s\t%s\t%s\t%s"%(p[0],p[1],gt,len(L),len(g60),len(g92),
        "yes" if (g60 and g92) else "NO",
        bst["i"] if bst else "-",
        "%.1f"%bst["rot"] if bst else "-",
        "%.1f"%bst["tn"] if bst else "-",
        ("%.2f"%cd(p[0],p[1])) if cd(p[0],p[1]) is not None else "not_touching"))

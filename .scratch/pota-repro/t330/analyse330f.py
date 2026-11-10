
import os
D=r"C:\PR\worktrees\structure-from-sherds-pp\330\.scratch\pota-repro\t330"
OUT=os.path.join(D,"out")
def parse(p):
    cand={};drop={};surv={};merged=set();meta={}
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
        elif k=="MERGE":
            if s[1]=="kept_idx": continue
            merged.add(int(s[2]))
    return dict(cand=cand,drop=drop,surv=surv,merged=merged,meta=meta)
a=parse(os.path.join(OUT,"audit_MOR1_S60.txt")); b=parse(os.path.join(OUT,"audit_MOR1_S9277.txt"))
by={}
for i in a["cand"]:
    c=a["cand"][i]; by.setdefault((min(c["y"],c["x"]),max(c["y"],c["x"])),[]).append(i)
def st(D,i):
    return "S" if i in D["surv"] else ("M" if i in D["merged"] else "D")
PAIRS=[(25,26),(24,31),(3,14),(3,30),(5,30),(17,24),(10,12),(3,23),(5,26),(13,31),(24,25),(1,9),(5,7),(6,19),(6,30),(11,13),(17,23),(18,24),(18,25),(26,27),(3,5)]
print("pair     gt  ncand  S60-surv  S9277-surv   best-GT-correct-cand (rot<t, t mm)")
for p in PAIRS:
    ids=by.get(p,[])
    gt=a["cand"][ids[0]]["gt"] if ids else -1
    s60=[i for i in ids if st(a,i)=="S"]; s92=[i for i in ids if st(b,i)=="S"]
    ok=[i for i in ids if a["cand"][i]["rot"]<20.05 and a["cand"][i]["tn"]<50.0]
    print("P%02d-P%02d %+d  %5d  %8d  %9d   %s"%(p[0],p[1],gt,len(ids),len(s60),len(s92),
        [ (i,round(a["cand"][i]["rot"],1),round(a["cand"][i]["tn"],1),st(a,i)+"/"+st(b,i)) for i in ok ]))
print()
print("=== detail for P25-P26 (the wrong contact) ===")
for i in by.get((25,26),[]):
    print(" idx%-4d gt=%d score %.3f->%.3f inl %d->%d area %.2f->%.2f axis %.3f gap %.2f  S60=%s S9277=%s drop60=%s"%(
        i,a["cand"][i]["gt"],a["cand"][i]["score"],b["cand"][i]["score"],a["cand"][i]["inlier"],b["cand"][i]["inlier"],
        a["cand"][i]["area"],b["cand"][i]["area"],a["cand"][i]["axis"],a["cand"][i]["gap"],st(a,i),st(b,i),a["drop"].get(i,"")))


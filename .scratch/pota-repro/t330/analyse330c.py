
import os,json
D=r"C:\PR\worktrees\structure-from-sherds-pp\330\.scratch\pota-repro\t330"
OUT=os.path.join(D,"out")
def parse(p):
    cand={};drop={};surv={};merges=[];meta={}
    for ln in open(p,errors="ignore"):
        s=ln.rstrip("\n").split("\t")
        if not s: continue
        k=s[0]
        if k=="SCALE": meta["scale"]=float(s[1])
        elif k=="TOTAL": meta["total"]=int(s[1])
        elif k=="PRUNED": meta["pruned"]=int(s[1])
        elif k=="CAND":
            if s[1]=="idx": continue
            i=int(s[1])
            cand[i]=dict(i=i,y=int(s[2]),x=int(s[3]),gt=int(s[4]),size=int(s[5]),score=float(s[6]),inlier=int(s[7]),
                overlap=int(s[8]),area=float(s[9]),axis=float(s[10]),rot=float(s[11]),tn=float(s[12]),
                rms=float(s[13]),gap=float(s[14]),dmed=float(s[15]))
        elif k=="DROP":
            if s[1]=="idx": continue
            drop[int(s[1])]=s[4]
        elif k=="SURVIVOR":
            if s[1]=="idx": continue
            surv[int(s[1])]=1
        elif k=="MERGE":
            if s[1]=="kept_idx": continue
            merges.append((int(s[1]),int(s[2])))
    return dict(cand=cand,drop=drop,surv=surv,merges=merges,meta=meta)
A={l:parse(os.path.join(OUT,"audit_%s.txt"%l)) for l in ["MOR1_S60","MOR1_S9277","K3R1_S60","K3R1_S9277"]}
a=A["MOR1_S60"]; b=A["MOR1_S9277"]
FAIL=[(3,5),(3,17),(3,24),(3,25),(3,26),(5,25),(13,25),(14,26),(14,30),(17,18),(18,23),(20,23),(23,24),(24,31),(26,30),(28,31)]
def key(i): c=a["cand"][i]; return (min(c["y"],c["x"]),max(c["y"],c["x"]))
by=dict()
for i in a["cand"]:
    by.setdefault(key(i),[]).append(i)
print("=== 16 failed joins: GT candidates at S60 vs S9277 (rot deg, t mm vs GT) ===")
for p in FAIL:
    ids=by.get(p,[])
    gts=[i for i in ids if a["cand"][i]["gt"]==1]
    ok60=[i for i in gts if a["cand"][i]["rot"]<20.05 and a["cand"][i]["tn"]<50.0]
    ok92=[i for i in ok92_ if False] if False else [i for i in gts if b["cand"][i]["rot"]<20.05 and b["cand"][i]["tn"]<50.0]
    good60=[i for i in ok60 if i in a["surv"]]
    good92=[i for i in ok92 if i in b["surv"]]
    print("P%02d-P%02d cands=%d gt=%d  gt-correct(t<50,rot<20)=%d  correct&surv S60=%s S9277=%s"%(p[0],p[1],len(ids),len(gts),len(ok60),
        [ (a["cand"][i]["i"],round(a["cand"][i]["rot"],2),round(a["cand"][i]["tn"],2)) for i in good60],
        [ (b["cand"][i]["i"],round(b["cand"][i]["rot"],2),round(b["cand"][i]["tn"],2)) for i in good92]))
    for i in gts:
        st60 = "S" if i in a["surv"] else ("M" if any(i==m[1] for m in a["merges"]) else "D")
        st92 = "S" if i in b["surv"] else ("M" if any(i==m[1] for m in b["merges"]) else "D")
        if st60!=st92 or a["cand"][i]["rot"]<20.05:
            print("    idx%-4d S60=%s S9277=%s rot %7.2f->%7.2f t %7.2f->%7.2f score %.3f->%.3f area %6.2f->%6.2f axis %.3f drop60=%s"%(
              i,st60,st92,a["cand"][i]["rot"],b["cand"][i]["rot"],a["cand"][i]["tn"],b["cand"][i]["tn"],
              a["cand"][i]["score"],b["cand"][i]["score"],a["cand"][i]["area"],b["cand"][i]["area"],a["cand"][i]["axis"],a["drop"].get(i,"(merged)" if st60=="M" else "")))


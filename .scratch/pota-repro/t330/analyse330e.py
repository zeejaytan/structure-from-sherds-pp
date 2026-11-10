
import os,numpy as np
D=r"C:\PR\worktrees\structure-from-sherds-pp\330\.scratch\pota-repro\t330"
OUT=os.path.join(D,"out")
def parse(p):
    cand={};drop={};surv={};merges=[];meta={}
    for ln in open(p,errors="ignore"):
        s=ln.rstrip("\n").split("\t")
        if not s: continue
        k=s[0]
        if k=="SCALE": meta["scale"]=float(s[1])
        elif k=="CAND":
            if s[1]=="idx": continue
            i=int(s[1])
            cand[i]=dict(i=i,y=int(s[2]),x=int(s[3]),gt=int(s[4]),score=float(s[6]),inlier=int(s[7]),
                area=float(s[9]),axis=float(s[10]),rot=float(s[11]),tn=float(s[12]))
        elif k=="DROP":
            if s[1]=="idx": continue
            drop[int(s[1])]=s[4]
        elif k=="SURVIVOR":
            if s[1]=="idx": continue
            surv[int(s[1])]=1
        elif k=="MERGE":
            if s[1]=="kept_idx": continue
            merges.append(dict(kept=int(s[1]),dropped=int(s[2]),y=int(s[3]),x=int(s[4]),ks=float(s[5]),ds=float(s[6]),rot=float(s[7]),t=float(s[8])))
    return dict(cand=cand,drop=drop,surv=surv,merges=merges,meta=meta)
for lane in ["MOR1","K3R1"]:
    a=parse(os.path.join(OUT,"audit_%s_S60.txt"%lane)); b=parse(os.path.join(OUT,"audit_%s_S9277.txt"%lane))
    k60=set((m["kept"],m["dropped"]) for m in a["merges"]); k92=set((m["kept"],m["dropped"]) for m in b["merges"])
    print("=== %s: merges S60=%d S9277=%d ; only-9277=%d only-60=%d"%(lane,len(a["merges"]),len(b["merges"]),len(k92-k60),len(k60-k92)))
    print("  (1-based pair, kept vs dropped candidate: gt/err of each, merge window usage)")
    for m in b["merges"]:
        if (m["kept"],m["dropped"]) in k92-k60:
            kc=b["cand"][m["kept"]]; dc=b["cand"][m["dropped"]]
            print("  NEWMERGE P%02d-P%02d kept idx%d(score %.2f rot %.1f t %.1f) <- dropped idx%d(score %.2f rot %.1f t %.1f) | window rot %.1fdeg t %.1fmm"%(
                m["kept"] and kc["y"],kc["x"],m["kept"],kc["score"],kc["rot"],kc["tn"],m["dropped"],dc["score"],dc["rot"],dc["tn"],m["rot"],m["t"]))
    for m in a["merges"]:
        if (m["kept"],m["dropped"]) in k60-k92:
            kc=a["cand"][m["kept"]]; dc=a["cand"][m["dropped"]]
            print("  LOSTMERGE P%02d-P%02d kept idx%d(score %.2f rot %.1f t %.1f) <- dropped idx%d(score %.2f rot %.1f t %.1f) | window rot %.1fdeg t %.1fmm"%(
                kc["y"],kc["x"],m["kept"],kc["score"],kc["rot"],kc["tn"],m["dropped"],dc["score"],dc["rot"],dc["tn"],m["rot"],m["t"]))


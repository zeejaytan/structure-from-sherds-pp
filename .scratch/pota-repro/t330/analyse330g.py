
import os,collections
D=r"C:\PR\worktrees\structure-from-sherds-pp\330\.scratch\pota-repro\t330"
OUT=os.path.join(D,"out")
def parse(p):
    cand={};drop={};surv={};merged={};meta={}
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
            merged[int(s[2])]=1
        elif k=="PRUNED": meta["pruned"]=int(s[1])
    return dict(cand=cand,drop=drop,surv=surv,merged=merged,meta=meta)
for lane in ["MOR1","K3R1"]:
    a=parse(os.path.join(OUT,"audit_%s_S60.txt"%lane)); b=parse(os.path.join(OUT,"audit_%s_S9277.txt"%lane))
    def cat(d,i):
        r=d["drop"].get(i,"")
        if r.startswith("overlap"): return "overlap"
        if r.startswith("basket_lowest_score"): return "basket_min_score"
        if r.startswith("axis_angle"): return "axis_angle"
        if i in d["merged"]: return "merged"
        return "surv"
    gained=[i for i in a["cand"] if i not in a["surv"] and i in b["surv"]]
    lost  =[i for i in a["cand"] if i in a["surv"] and i not in b["surv"]]
    print("##",lane,"pruned S60=%s S9277=%s  gained=%d lost=%d net=%+d"%(a["meta"].get("pruned"),b["meta"].get("pruned"),len(gained),len(lost),len(gained)-len(lost)))
    for name,L in [("GAIN",gained),("LOST",lost)]:
        c=collections.Counter(cat(a,i) for i in L)
        print("   %s by S60 fate: %s"%(name,dict(c)))
        gtc=sum(1 for i in L if a["cand"][i]["gt"]==1)
        junk=sum(1 for i in L if a["cand"][i]["rot"]>90)
        print("      of which GT pairs=%d, rot>90deg (junk poses)=%d"%(gtc,junk))
    # overlap-window specific: S60 dropped by overlap with area in (50,77.31]
    ow=[i for i in gained if cat(a,i)=="overlap" and 50.0 < a["cand"][i]["area"] <= 77.31]
    print("   gained exactly by the widened overlap window (50<area<=77.31):",len(ow))
    mw=[i for i in gained if cat(a,i)=="overlap" and a["cand"][i]["area"] > 77.31]
    print("   gained though area>77.31 (pose/area changed, not the window):",len(mw))


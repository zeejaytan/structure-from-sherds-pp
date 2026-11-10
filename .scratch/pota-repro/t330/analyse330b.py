
import os,json,collections
D=os.path.dirname(os.path.abspath("x")) 
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
            i=int(s[1]); y=int(s[2]); x=int(s[3])
            cand[i]=dict(i=i,y=y,x=x,gt=int(s[4]),size=int(s[5]),score=float(s[6]),inlier=int(s[7]),
                overlap=int(s[8]),area=float(s[9]),axis=float(s[10]),rot=float(s[11]),tn=float(s[12]),
                rms=float(s[13]),gap=float(s[14]),dmed=float(s[15]))
        elif k=="DROP":
            if s[1]=="idx": continue
            drop[int(s[1])]=s[4]
        elif k=="SURVIVOR":
            if s[1]=="idx": continue
            surv[int(s[1])]=dict(gt=int(s[4]),score=float(s[5]),inlier=int(s[6]),area=float(s[7]),size=int(s[12].split()[0]))
        elif k=="MERGE":
            if s[1]=="kept_idx": continue
            merges.append((int(s[1]),int(s[2])))
    return dict(cand=cand,drop=drop,surv=surv,merges=merges,meta=meta)
A={l:parse(os.path.join(OUT,"audit_%s.txt"%l)) for l in ["MOR1_S60","MOR1_S9277","K3R1_S60","K3R1_S9277"]}
for l in A: print("##",l,"scale",A[l]["meta"].get("scale"),"total",len(A[l]["cand"]),"surv",len(A[l]["surv"]),"drop",len(A[l]["drop"]),"merges",len(A[l]["merges"]))
for lane in ["MOR1","K3R1"]:
    a=A[lane+"_S60"]; b=A[lane+"_S9277"]
    same = all(a["cand"][i]["y"]==b["cand"][i]["y"] and a["cand"][i]["x"]==b["cand"][i]["x"] and a["cand"][i]["gt"]==b["cand"][i]["gt"] for i in a["cand"] if i in b["cand"])
    print("==",lane,"enumeration-identical:",same,"ids",len(a["cand"]),len(b["cand"]))
    # merge set: kept + survivors
    flips=[]
    for i in a["cand"]:
        va = "SURV" if i in a["surv"] else "DROP"
        vb = "SURV" if i in b["surv"] else "DROP"
        if va!=vb: flips.append(i)
    print("   flips:",len(flips), "gt flips:",sum(1 for i in flips if a["cand"][i]["gt"]==1))
    for i in flips:
        c=a["cand"][i]; cb=b["cand"][i]
        print("   FLIP %-3d P%02d-P%02d gt=%d  %s->%s  score %.3f->%.3f inl %d->%d area %.2f->%.2f axis %.3f gap %.2f | drop60=%s"%(
            i,c["y"],c["x"],c["gt"],"SURV" if i in a["surv"] else "DROP","SURV" if i in b["surv"] else "DROP",
            c["score"],cb["score"],c["inlier"],cb["inlier"],c["area"],cb["area"],c["axis"],c["gap"],a["drop"].get(i,"")))


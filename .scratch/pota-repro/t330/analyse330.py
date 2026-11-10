import os, sys, json, math, collections
import numpy as np

D = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(D, "out")
GTD = os.path.join(D, "data", "x_gt", "Ground Truth")

def parse(path):
    cand={}; drop={}; surv={}; merges=[]; meta={}
    for ln in open(path, errors="ignore"):
        s=ln.rstrip("\n").split("\t")
        if not s: continue
        k=s[0]
        if k=="SCALE": meta["scale"]=float(s[1]); meta["scale_line"]=ln.strip()
        elif k=="TOTAL": meta["total"]=int(s[1])
        elif k=="PRUNED": meta["pruned"]=int(s[1])
        elif k=="CAND":
            if s[1]=="idx": continue
            y=int(s[2]); x=int(s[3])
            cand[(min(y,x),max(y,x))]=dict(idx=int(s[1]),y=y,x=x,gt=int(s[4]),size=int(s[5]),score=float(s[6]),
                inlier=int(s[7]),overlap=int(s[8]),area=float(s[9]),axis=float(s[10]),rot=float(s[11]),
                tn=float(s[12]),rms=float(s[13]),gap=float(s[14]),dmed=float(s[15]),nx=int(s[16]),ny=int(s[17].split()[0]),
                trans=[float(v) for v in s[17].split()[1:17]])
        elif k=="DROP":
            if s[1]=="idx": continue
            drop[(min(int(s[2]),int(s[3])),max(int(s[2]),int(s[3])))]=(int(s[1]),s[4])
        elif k=="SURVIVOR":
            if s[1]=="idx": continue
            y=int(s[2]); x=int(s[3])
            surv[(min(y,x),max(y,x))]=dict(idx=int(s[1]),gt=int(s[4]),score=float(s[5]),inlier=int(s[6]),
                area=float(s[7]),axis=float(s[8]),rot=float(s[9]),tn=float(s[10]),rms=float(s[11]),size=int(s[12].split()[0]))
        elif k=="MERGE":
            if s[1]=="kept_idx": continue
            merges.append(dict(kept=int(s[1]),dropped=int(s[2]),y=int(s[3]),x=int(s[4]),ks=float(s[5]),ds=float(s[6]),
                               rot=float(s[7]),t=float(s[8]),rms=float(s[9])))
    return dict(cand=cand,drop=drop,surv=surv,merges=merges,meta=meta)

def P(lbl):
    return parse(os.path.join(OUT,"audit_%s.txt"%lbl))

def gate_of(a, b):
    """name the threshold whose value differs between lit and gate(x) and which the pair's numbers straddle"""
    out=[]
    if a is None: return out
    if a.get("area") is not None and a["area"]>50.0 and a["area"]<=77.31: out.append(("overlap area 50.0->77.31", a["area"]))
    return out

def main():
    labels=[l for l in ["MOR1_S60","MOR1_S9277","K3R1_S60","K3R1_S9277"]
            if os.path.exists(os.path.join(OUT,"audit_%s.txt"%l))]
    A={l:P(l) for l in labels}
    for l in labels:
        m=A[l]["meta"]
        print("## %-11s S=%-7s TOTAL=%-5s PRUNED=%-5s cand=%-5d drop=%-5d surv=%-5d merge=%d"%(
            l,m.get("scale"),m.get("total"),m.get("pruned"),len(A[l]["cand"]),len(A[l]["drop"]),len(A[l]["surv"]),len(A[l]["merges"])))
    for lane in ["MOR1","K3R1"]:
        a,b=lane+"_S60",lane+"_S9277"
        if a not in A or b not in A: continue
        s60,s9277=A[a]["surv"],A[b]["surv"]
        d60=A[a]["drop"]; c=A[b]["cand"]
        gained=sorted(set(s9277)-set(s60))
        lost=sorted(set(s60)-set(s9277))
        print("\n=== %s flips: gained(survives only at 92.77)=%d lost(survives only at 60)=%d" % (lane,len(gained),len(lost)))
        for kind, lst in (("GAIN",gained),("LOST",lost)):
            for k in lst:
                r=c.get(k) or A[a]["cand"].get(k)
                why=d60.get(k,("",""))[1] if kind=="GAIN" else A[b]["drop"].get(k,("",""))[1]
                if r is None:
                    print("  %s P%02d-P%02d  (no CAND record)  drop60=%s" % (kind,k[0],k[1],why)); continue
                print("  %s P%02d-P%02d gt=%d score=%.3f inl=%d area=%.2f axis=%.3f rot=%.2f t=%.2f rms=%.2f gap=%.3f med=%.2f drop60=%s"%(
                    kind,k[0],k[1],r["gt"],r["score"],r["inlier"],r["area"],r["axis"],r["rot"],r["tn"],r["rms"],r["gap"],r["dmed"],why))
    # merge-set diffs
    for lane in ["MOR1","K3R1"]:
        a,b=lane+"_S60",lane+"_S9277"
        if a not in A or b not in A: continue
        m60={(m["y"],m["x"]) for m in A[a]["merges"]}; m92={(m["y"],m["x"]) for m in A[b]["merges"]}
        print("\n=== %s merges: S60=%d S92.77=%d  only-in-92.77=%s" % (lane,len(m60),len(m92),
              sorted({(min(y,x),max(y,x)) for (y,x) in (m92-m60)})))
main()

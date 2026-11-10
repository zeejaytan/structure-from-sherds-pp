
import os,numpy as np,math
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
            i=int(s[1]); tr=[float(v) for v in s[17].split()[1:17]]
            cand[i]=dict(i=i,y=int(s[2]),x=int(s[3]),gt=int(s[4]),score=float(s[6]),inlier=int(s[7]),
                area=float(s[9]),axis=float(s[10]),rot=float(s[11]),tn=float(s[12]),gap=float(s[14]),
                M=np.array(tr,dtype=float).reshape(4,4))
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
A={l:parse(os.path.join(OUT,"audit_%s.txt"%l)) for l in ["MOR1_S60","MOR1_S9277"]}
a=A["MOR1_S60"]; b=A["MOR1_S9277"]
def readT(p):
    v=[float(x) for x in open(p).read().split()]
    return np.array(v,dtype=float).reshape(4,4)
def poses(sub):
    P={}
    for i in range(31):
        p=os.path.join(D,"data",sub,"T_%dto_%d.txt"%(i,i))
        if os.path.exists(p): P[i]=readT(p)
    return P
S=poses("x_t326_mor1_out"); S0=poses("x_t323_mor1_out")
def rel(P,i,j):
    return np.linalg.inv(P[i])@P[j]
def xerr(M,N):
    R=M[:3,:3].T@N[:3,:3]; c=max(-1,min(1,(np.trace(R)-1)/2)); ang=math.degrees(math.acos(c)); t=np.linalg.norm(M[:3,3]-N[:3,3]); return ang,t
by=dict()
for i in a["cand"]:
    c=a["cand"][i]; by.setdefault((min(c["y"],c["x"]),max(c["y"],c["x"])),[]).append(i)
FAIL=[(3,5),(3,17),(3,24),(3,25),(3,26),(5,25),(13,25),(14,26),(14,30),(17,18),(18,23),(20,23),(23,24),(24,31),(26,30),(28,31)]
# sanity: check convention on a few GT pairs at t323
print("=== convention check: |cand.trans_ - inv(T_i)T_j| at t323 (should be ~0 for the used candidate) ===")
for p in [(1,2),(3,17),(3,24),(5,8)]:
    ids=by.get(p,[])
    M=rel(S0,p[0]-1,p[1]-1)
    best=sorted((xerr(a["cand"][i]["M"],M)[0]+0.0,xerr(a["cand"][i]["M"],M)[1],i) for i in ids)[:2]
    print("P%02d-P%02d"%p, [ (i,round(dr,2),round(dt,2)) for dr,dt,i in best])
print("=== t326 final relative pose vs each candidate (best match) ===")
for p in FAIL:
    ids=by.get(p,[])
    M=rel(S,p[0]-1,p[1]-1)
    rows=sorted((xerr(a["cand"][i]["M"],M)[0],xerr(a["cand"][i]["M"],M)[1],i) for i in ids)[:2]
    out=[]
    for dr,dt,i in rows:
        st60="S" if i in a["surv"] else ("M" if any(i==m[1] for m in a["merges"]) else "D")
        st92="S" if i in b["surv"] else ("M" if any(i==m[1] for m in b["merges"]) else "D")
        out.append("idx%d d=%.2fdeg/%.2fmm S60=%s S9277=%s %s"%(i,dr,dt,st60,st92,a["drop"].get(i,"")))
    print("P%02d-P%02d gt=%d | %s"%(p[0],p[1],a["cand"][ids[0]]["gt"] if ids else -1," || ".join(out)))


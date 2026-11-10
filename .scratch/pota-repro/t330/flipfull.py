
import json,os
D=r"C:\PR\worktrees\structure-from-sherds-pp\330\.scratch\pota-repro\t330"
OUT=os.path.join(D,"out")
def parse(p):
    cand={};drop={};surv={};merged={}
    for ln in open(p,errors="ignore"):
        s=ln.rstrip("\n").split("\t")
        if not s: continue
        k=s[0]
        if k=="CAND":
            if s[1]=="idx": continue
            i=int(s[1]); ny=int(s[17].split()[0])
            cand[i]=dict(i=i,y=int(s[2]),x=int(s[3]),gt=int(s[4]),score=float(s[6]),inlier=int(s[7]),
                area=float(s[9]),axis=float(s[10]),rot=float(s[11]),tn=float(s[12]),gap=float(s[14]),dmed=float(s[15]))
        elif k=="DROP":
            if s[1]=="idx": continue
            drop[int(s[1])]=s[4]
        elif k=="SURVIVOR":
            if s[1]=="idx": continue
            surv[int(s[1])]=1
        elif k=="MERGE":
            if s[1]=="kept_idx": continue
            merged[int(s[2])]=1
    return cand,drop,surv,merged
ca,da,sa,ma=parse(os.path.join(OUT,"audit_MOR1_S60.txt"))
cb,db,sb,mb=parse(os.path.join(OUT,"audit_MOR1_S9277.txt"))
def grouped(cand,surv):
    g={}
    for i,c in cand.items():
        if i in surv: g.setdefault((c["y"],c["x"]),[]).append(i)
    return g
ga=grouped(ca,sa); gb=grouped(cb,sb)
rows=[]
for p in sorted(set(ga)|set(gb)):
    if bool(ga.get(p))==bool(gb.get(p)):
        continue
    direction="accept" if gb.get(p) else "reject"
    ids=gb.get(p) or ga.get(p)
    i=min(ids,key=lambda k:(ca.get(k) or cb.get(k))["score"])
    c=cb.get(i) or ca.get(i)
    rows.append((p,direction,c["gt"],c["inlier"],c["gap"],c["dmed"],c["score"],c["area"],c["axis"],
                 da.get(i,"-"),db.get(i,"-")))
print("pair dir gt inl gap dmed score area axis drop60 drop9277")
for r in rows:
    print("%02d-%02d %s gt=%d inl=%d gap=%.2f dmed=%.2f score=%.3f area=%.1f axis=%.3f | %s | %s"%(r[0][0],r[0][1],r[1],r[2],r[3],r[4],r[5],r[6],r[7],r[8],r[9],r[10]))

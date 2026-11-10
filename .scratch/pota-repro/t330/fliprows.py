
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
            cand[i]=dict(i=i,y=int(s[2]),x=int(s[3]),gt=int(s[4]),size=int(s[5]),score=float(s[6]),
                inlier=int(s[7]),overlap=int(s[8]),area=float(s[9]),axis=float(s[10]),rot=float(s[11]),
                tn=float(s[12]),rms=float(s[13]),gap=float(s[14]),dmed=float(s[15]),nx=int(s[16]),ny=ny)
        elif k=="DROP":
            if s[1]=="idx": continue
            drop[int(s[1])]=s[4]
        elif k=="SURVIVOR":
            if s[1]=="idx": continue
            surv[int(s[1])]=1
    return cand,drop,surv
contacts=json.load(open(os.path.join(D,"contacts.json")))
def cdist(d,a,b):
    k=(min(a,b),max(a,b))
    return d.get("%d-%d"%k, d.get(k))
for lane in ["MOR1"]:
    ca,da,sa=parse(os.path.join(OUT,"audit_%s_S60.txt"%lane))
    cb,db,sb=parse(os.path.join(OUT,"audit_%s_S9277.txt"%lane))
    def bypair(cand,surv):
        m={}
        for i in cand:
            if i in surv: m.setdefault((cand[i]["y"],cand[i]["x"]),[]).append(i)
        return m
    pa=bypair(ca,sa); pb=bypair(cb,sb)
    allp=sorted(set(pa)|set(pb))
    print("pair\tgt\tS60_surv\tS9277_surv\tbest idx\tinl\tgap_min\td_med\tscore\trot\ttn\tdrop_at_60")
    for p in allp:
        A=pa.get(p); B=pb.get(p)
        if bool(A)==bool(B): continue
        which=B or A
        i=min(which,key=lambda k:(ca.get(k,cb.get(k))["score"]))
        c=cb.get(i) or ca.get(i)
        print("%d-%d\t%d\t%d\t%d\t%d\t%d\t%.2f\t%.2f\t%.3f\t%.2f\t%.2f\t%s"%(
            p[0],p[1],c["gt"],1 if A else 0,1 if B else 0,i,c["inlier"],c["gap"],c["dmed"],
            c["score"],c["rot"],c["tn"], da.get(i,"-")))

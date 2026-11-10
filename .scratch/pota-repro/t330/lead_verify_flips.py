
import os, collections
D = r"C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330/lead_audit"
HDR="idx y x gt size score inlier overlap area axis_angle rot t rms gap dmed npx npy".split()

def gt_edges(path):
    rows=[l.split() for l in open(path) if l.strip()]
    return {(min(i+1,j+1),max(i+1,j+1)) for i,r in enumerate(rows[:31]) for j,v in enumerate(r) if v=='1'}

def load(p):
    rows={}; drop={}; surv=set(); merge=set()
    for line in open(p):
        f=line.rstrip("\n").split("\t")
        if len(f)<2 or not f[1].strip().isdigit(): continue
        i=int(f[1])
        if f[0]=='CAND': rows[i]=f
        elif f[0]=='DROP': drop[i]=f[4]
        elif f[0]=='SURVIVOR': surv.add(i)
        elif f[0]=='MERGE': merge.add(i)
    return rows,drop,surv,merge

E=gt_edges(os.path.join(D,'gt_graph.txt'))
for lane in ('MOR1','K3R1'):
    c,dr60,s60,m60=load(os.path.join(D,'audit_%s_S60.txt'%lane))
    _,dr92,s92,m92=load(os.path.join(D,'audit_%s_S9277.txt'%lane))
    def pair(i): f=c[i]; return (min(int(f[2]),int(f[3])),max(int(f[2]),int(f[3])))
    allp=collections.defaultdict(lambda:[0,0])
    for i in c:
        allp[pair(i)][0]+= 1 if i in s60 else 0
        allp[pair(i)][1]+= 1 if i in s92 else 0
    d1={p for p,(a,b) in allp.items() if (a>0)!=(b>0)}
    print("--- %s: pairs total %d | D1 (has>=1 survivor differs): %d ; of which GT %d"%(lane,len(allp),len(d1),len([p for p in d1 if p in E])))
    print("    D1 pairs:",sorted(d1))
    # reason table for flipped candidates
    cnt=collections.Counter()
    for i in (s60^s92):
        r60='SURV' if i in s60 else dr60.get(i,'?'); r92='SURV' if i in s92 else dr92.get(i,'?')
        cnt[(r60.split('(')[0],r92.split('(')[0])]+=1
    print("    candidate-level reason transitions:")
    for k,v in cnt.most_common(): print("      %-18s -> %-18s %d"%(k[0],k[1],v))
    if lane=='MOR1':
        for want in [(3,26),(25,26),(25,31),(26,31)]:
            ks=[i for i in c if pair(i)==want]
            print("    pair %s: %d candidates"%(want,len(ks)))
            for i in sorted(ks):
                f=c[i]; print("      idx %-5d gt=%s score=%.3f inl=%s gap=%s rot=%s t=%s | S60=%s S9277=%s"%(i,f[4],float(f[6]),f[7],f[14],f[10],f[11],'SURV' if i in s60 else dr60.get(i,'?'),'SURV' if i in s92 else dr92.get(i,'?')))
    print("    merge diff pairs:",sorted({pair(i) for i in (m60^m92)}),"gt flags:",[(p,p in E) for p in sorted({pair(i) for i in (m60^m92)})])

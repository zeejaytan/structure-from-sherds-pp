
import re
rows={}
hdr=None
for ln in open("diag_juglet.log", errors="ignore"):
    f=ln.rstrip("\n").split("\t")
    if f[0]=="DISP" and f[1]=="idx": hdr=f; continue
    if f[0]=="DISP" and hdr: rows[int(f[1])]=dict(zip(hdr[1:],f[1:]))
def g(i,k):
    return float(rows[i][k])
want={1:"1-2",5:"1-2",70:"1-8",69:"1-8",77:"1-9",81:"1-9",174:"7-9",175:"7-9",1:"1-2",3:"1-2"}
print("idx pair  ncor   dist_sum  norm_sum   mean(dist+norm)/n   shipped (d+n)/n^1.2   gates")
for i in sorted(want):
    r=rows[i]; n=g(i,"ncor"); d=g(i,"dist_sum"); nm=g(i,"norm_sum")
    if n<=0: continue
    print(f"{i:4d} {want[i]:5s} {int(n):5d} {d:9.3f} {nm:8.3f}   {(d+nm)/n:8.4f}          {(d+nm)/n**1.2:8.4f}   {r['erased_by']}")
print()
for i in (1,5):
    r=rows[i]; n=g(i,"ncor"); d=g(i,"dist_sum"); nm=g(i,"norm_sum")
    print(f"1-2 idx={i} ncor={int(n)} ratio n^1.2={n**1.2:.1f}")

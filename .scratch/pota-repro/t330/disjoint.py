
import json,os
D=r"C:\PR\worktrees\structure-from-sherds-pp\330\.scratch\pota-repro\t330"
ct=json.load(open(os.path.join(D,"contacts.json")))
M=ct["x_t326_mor1_out"]; M0=ct["x_t323_mor1_out"]
import math
n=31
def touch(M,i,j):
    v=M[i-1][j-1]
    return (not (isinstance(v,float) and math.isnan(v))) and v<=1.0
flips={(1,17),(2,23),(3,15),(4,27),(5,24),(8,25),(13,30),(15,24),(15,30),(18,29),(20,27),(22,26),(24,27)}
bad={(3,5),(3,17),(3,24),(3,25),(3,26),(5,25),(13,25),(14,26),(14,30),(17,18),(18,23),(20,23),(23,24),(24,31),(26,30),(28,31)}
norm=lambda p:(min(p),max(p))
flips={norm(p) for p in flips}; bad={norm(p) for p in bad}
print("flips:",sorted(flips))
print("flips ∩ failed joins:",sorted(flips&bad))
newt=[(i,j) for i in range(1,n+1) for j in range(i+1,n+1) if touch(M,i,j) and not touch(M0,i,j)]
print("new <=1mm contacts in t326:",newt)
print("flips ∩ new contacts:",sorted(flips&{norm(p) for p in newt}))
gone=[(i,j) for i in range(1,n+1) for j in range(i+1,n+1) if touch(M0,i,j) and not touch(M,i,j)]
print("contacts lost in t326:",gone)
print("flips ∩ lost contacts:",sorted(flips&{norm(p) for p in gone}))
for s in (3,24,25,26,31):
    nb=[j for j in range(1,n+1) if j!=s and touch(M,s,j)]
    print("P%02d t326 touching: %s"%(s,nb))


import json,os,math
D=r"C:\PR\worktrees\structure-from-sherds-pp\330\.scratch\pota-repro\t330"
ct=json.load(open(os.path.join(D,"contacts.json")))
M=ct["x_t326_mor1_out"]; M0=ct["x_t323_mor1_out"]
def cd(M,a,b):
    v=M[a-1][b-1]
    return v
bad=[(3,5),(3,17),(3,24),(3,25),(3,26),(5,25),(13,25),(14,26),(14,30),(17,18),(18,23),(20,23),(23,24),(24,31),(26,30),(28,31)]
print("join  contact_t323  contact_t326")
for (a,b) in bad:
    print("%02d-%02d   %8.2f   %8.2f"%(a,b,cd(M0,a,b),cd(M,a,b)))

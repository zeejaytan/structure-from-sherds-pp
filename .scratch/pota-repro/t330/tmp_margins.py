import json, numpy as np
rows=json.load(open(r"C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330/edge_err.json"))
for tag in ("t323_mor1_out","t326_mor1_out"):
    R=[r for r in rows if r[0]==tag]
    ok=[r for r in R if r[5]]; bad=[r for r in R if not r[5]]
    ang=np.array([r[3] for r in ok]); tr=np.array([r[4] for r in ok])
    print(tag,"pass",len(ok),"fail",len(bad),"| passing max ang %.2f med %.2f ; max tr %.2f med %.2f"%(ang.max(),np.median(ang),tr.max(),np.median(tr)))
    print("   failing ang sorted:", [round(r[3],1) for r in sorted(bad,key=lambda x:x[3])])
print()
for r in rows:
    if (r[1],r[2])==(17,18):
        print(r)

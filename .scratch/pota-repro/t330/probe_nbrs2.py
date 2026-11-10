import os, json
import numpy as np
D = r"C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330"
C=json.load(open(os.path.join(D,"contacts.json")))
A=np.array(C["x_t326_mor1_out"],dtype=float); np.fill_diagonal(A,np.nan)
for p in [30,26,27]:
    row=sorted([(j+1,round(float(A[p,j]),2)) for j in range(31) if A[p,j]<=5.0])
    print("P%02d nearest<=5mm: %s" % (p+1,row))

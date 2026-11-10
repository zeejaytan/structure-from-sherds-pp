import json, numpy as np
D=r"C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330"
A=json.load(open(D+"/contacts.json"))["x_t323_mor1_out"]
M=np.array(A)
T=np.triu(M,1)
print(M.shape, T.shape, T.size, np.sum(T<=1.0), int(np.sum(T<=1.0)))
idx=np.argwhere(T<=1.0)
print("first idx", idx[:5])
print("max idx", idx.max(0))
print("pairs", len(idx))

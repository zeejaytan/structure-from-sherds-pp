import os, glob, math, json, re
import numpy as np
D = r"C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330"
def pts(d):
    out={}
    for p in glob.glob(os.path.join(d,"*_Edgeline_*.xyz")):
        k=int(os.path.basename(p)[:-4].split("_")[-1])
        A=np.loadtxt(p)
        if A.ndim==1: A=A.reshape(1,-1)
        out[k-1]=A[:,:3]
    return out
def contacts(P, cap=1e18):
    ks=sorted(P)
    n=max(ks)+1
    C=np.full((n,n), np.nan)
    for a in ks:
        for b in ks:
            if b<=a: continue
            A=P[a]; B=P[b]
            d=np.sqrt(((A[:,None,:]-B[None,:,:])**2).sum(-1)).min()
            C[a,b]=C[b,a]=d
    return C
res={}
for lab in ["x_t323_mor1_out","x_t326_mor1_out"]:
    P=pts(os.path.join(D,"data",lab))
    C=contacts(P)
    res[lab]=C.tolist()
    vals=C[~np.isnan(C)]
    print(lab,"pieces",len(P),"pair min-dist quantiles", np.round(np.quantile(vals,[0.01,0.05,0.1,0.2,0.5]),3),
          "count<=1mm",int((vals<=1).sum()),"<=2",int((vals<=2).sum()),"<=3",int((vals<=3).sum()),"<=5",int((vals<=5).sum()))
json.dump(res, open(os.path.join(D,"contacts.json"),"w"))

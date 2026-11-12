import numpy as np
def load_xyz(p):
    a=np.loadtxt(p); return a[:,:3], a[:,3:6]
def perp(P,p0,d):
    d=d/np.linalg.norm(d); q=P-p0; t=q@d; return q-np.outer(t,d)
print("piece  S0 frac normals->axis   S1 frac->axis   r(S1)-r(S0) mm   S0 wall (3x)   328")
s328={1:"inner",2:"outer",3:"inner",4:"inner",5:"inner",6:"outer",7:"inner",8:"inner",9:"outer"}
for i in range(1,10):
    a=np.loadtxt(f"Juglet_Piece_{i}_Axis.xyz").ravel(); p0,d=a[:3],a[3:6]
    fr=[]; rr=[]
    for s in (0,1):
        P,N=load_xyz(f"Juglet_Piece_{i}_Surface_{s}.xyz")
        m=np.linalg.norm(N,axis=1)>1e-9; P,N=P[m],N[m]
        u=perp(P,p0,d); r=np.linalg.norm(u,axis=1); rr.append(np.median(r))
        fr.append(float(((N*(u/r[:,None])).sum(1)<0).mean()))
    w="inner" if fr[0]>0.5 else "outer"
    print(f"{i:5d}   {fr[0]:.3f}                    {fr[1]:.3f}        {rr[1]-rr[0]:+7.2f}       {w:5s}          {s328[i]:5s} {'AGREE' if w==s328[i] else 'DISAGREE'}")
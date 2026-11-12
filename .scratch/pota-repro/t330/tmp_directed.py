import numpy as np, os, glob, re
DATA=r"C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330/data"
def read_T(path):
    lines=[l for l in open(path).read().splitlines() if l.strip()]
    M=np.eye(4)
    for i in range(len(lines)-1):
        v=lines[i].split(); M[i,:]=[float(x) for x in v[:4]]
    return M
def rot_angle(R):
    c=(np.trace(R)-1.0)/2.0; c=max(-1.0,min(1.0,c)); return float(np.arccos(c))
def similar(A,B,rad=0.35,tmm=50.0):
    T=A@np.linalg.inv(B); return rot_angle(T[:3,:3])<rad and float(np.linalg.norm(T[:3,3]))<tmm
def load_run(d):
    S={}
    for f in glob.glob(os.path.join(d,"T_*to_*.txt")):
        m=re.match(r"T_(\d+)to_(\d+)\.txt",os.path.basename(f)); S[int(m.group(1))]=read_T(f)
    return S
gd=os.path.join(DATA,"x_gt","Ground Truth")
G=np.loadtxt([f for f in glob.glob(os.path.join(gd,"*.txt")) if "Piece" not in os.path.basename(f)][0])
GT={}
for f in glob.glob(os.path.join(gd,"Pot_E_Piece_*_T.txt")):
    GT[int(re.search(r"Piece_(\d+)_T",os.path.basename(f)).group(1))-1]=read_T(f)
S=load_run(os.path.join(DATA,"x_t326_mor1_out"))
k=0; total=0; asym=[]; passes=set()
for i in range(31):
    for j in range(31):
        if G[i,j]==0: continue
        total+=1
        Gt=np.linalg.inv(GT[i])@GT[j]; T=np.linalg.inv(S[i])@S[j]
        if similar(Gt,T): k+=1; passes.add((i+1,j+1))
print("k",k,"total",total,"pairs",len(passes))
for (a,b) in sorted(passes):
    if (b,a) not in passes: print("asym",a,b)
# boundary check
for i in range(31):
    for j in range(31):
        if G[i,j]==0 or i>=j: continue
        Gt=np.linalg.inv(GT[i])@GT[j]; T=np.linalg.inv(S[i])@S[j]
        D=Gt@np.linalg.inv(T); ang=np.degrees(rot_angle(D[:3,:3])); tr=float(np.linalg.norm(D[:3,3]))
        if 15<ang<25 or 45<tr<60: print("near P%02d P%02d %.2fdeg %.2fmm pass=%s"%(i+1,j+1,ang,tr,similar(Gt,T)))

import numpy as np, os, glob, re
DATA=r"C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330/data"
def read_T(path):
    lines=[l for l in open(path).read().splitlines() if l.strip()]
    M=np.eye(4)
    for i in range(len(lines)-1):
        v=lines[i].split(); M[i,:]=[float(x) for x in v[:4]]
    return M
gd=os.path.join(DATA,"x_gt","Ground Truth")
GT={}
for f in glob.glob(os.path.join(gd,"Pot_E_Piece_*_T.txt")):
    GT[int(re.search(r"Piece_(\d+)_T",os.path.basename(f)).group(1))-1]=read_T(f)
def load_run(d):
    S={}
    for f in glob.glob(os.path.join(d,"T_*to_*.txt")):
        m=re.match(r"T_(\d+)to_(\d+)\.txt",os.path.basename(f)); S[int(m.group(1))]=read_T(f)
    return S
S=load_run(os.path.join(DATA,"x_t326_mor1_out"))
np.set_printoptions(precision=6,suppress=False)
for (i,j) in [(16,17),(17,16)]:
    Gt=np.linalg.inv(GT[i])@GT[j]; T=np.linalg.inv(S[i])@S[j]; D=Gt@np.linalg.inv(T)
    c=(np.trace(D[:3,:3])-1)/2
    print(i+1,j+1,"trace",np.trace(D[:3,:3]),"c",c,"ang_deg",np.degrees(np.arccos(max(-1,min(1,c)))),"t",D[:3,3],"norm",np.linalg.norm(D[:3,3]))
print()
print("S16 det",np.linalg.det(S[16][:3,:3]),"S17 det",np.linalg.det(S[17][:3,:3]))
print("GT16 det",np.linalg.det(GT[16][:3,:3]),"GT17 det",np.linalg.det(GT[17][:3,:3]))

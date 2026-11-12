#!/usr/bin/env python3
"""Self-test for trace336.py: synthetic log + GT, four classes plus a non-identity axis.

The non-identity-axis case pins the composition order: the saved pose is F = T_graph * T_axis
(Trans::Input pre-multiplies), so the graph pose recovered from FINAL must be F * inv(T_axis).
With the opposite order the phase match would be off by the axis rotation and the case would
report class D/'none' instead of init.
"""
import math, os, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
TRACE = os.path.join(HERE, "trace336.py")

def mat12(M):
    return "\t".join("%.9f" % M[r][c] for r in range(3) for c in range(4))

def mul(A, B):
    return [[sum(A[r][k] * B[k][c] for k in range(4)) for c in range(4)] for r in range(4)]

def inv(T):
    R = [[T[r][c] for c in range(3)] for r in range(3)]
    t = [T[r][3] for r in range(3)]
    Rt = [[R[c][r] for c in range(3)] for r in range(3)]
    ti = [-sum(Rt[r][k] * t[k] for k in range(3)) for r in range(3)]
    return [[Rt[r][c] for c in range(3)] + [ti[r]] for r in range(3)] + [[0.0, 0.0, 0.0, 1.0]]

def rotz(deg, t=(0.0, 0.0, 0.0)):
    a = math.radians(deg)
    c, s = math.cos(a), math.sin(a)
    return [[c, -s, 0, t[0]], [s, c, 0, t[1]], [0, 0, 1, t[2]], [0, 0, 0, 1]]

I = [[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]

def w(path, text):
    with open(path, "w") as f:
        f.write(text)

def case(name, lines, axis1=None, axis2=None, gt1=None, gt2=None, final1=None, final2=None,
         node_lines=None):
    axis1 = axis1 or I
    axis2 = axis2 or I
    gt1 = gt1 or I
    # default: piece 2's GT relation is a 100 mm translation, axis identity, graph pose = that
    g2 = gt2 if gt2 is not None else [[1, 0, 0, 100.0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]
    f1 = final1 or mul(axis1, I)      # T_graph=I -> F = I * T_axis
    f2 = final2 if final2 is not None else g2
    d = tempfile.mkdtemp(prefix="t336_" + name + "_")
    gtd = os.path.join(d, "Ground Truth"); os.makedirs(gtd)
    w(os.path.join(gtd, "Juglet_Piece_1_T.txt"), "\n".join(" ".join("%.9f" % x for x in r) for r in gt1) + "\n")
    w(os.path.join(gtd, "Juglet_Piece_2_T.txt"), "\n".join(" ".join("%.9f" % x for x in r) for r in g2) + "\n")
    log = ["[SFS-336]\tAXIS\t1\t" + mat12(axis1), "[SFS-336]\tAXIS\t2\t" + mat12(axis2)]
    log += lines
    if node_lines:
        log += node_lines
    log.append("[SFS-336]\tOUTSTATE\t0\t0.9000\t1")
    log.append("[SFS-336]\tCLUSTER\t0\t1\t0.9000\t1,2")
    log.append("[SFS-336]\tFINAL\t1\t0\t" + mat12(f1))
    log.append("[SFS-336]\tFINAL\t2\t0\t" + mat12(f2))
    p = os.path.join(d, "binary.log")
    w(p, "\n".join(log) + "\n")
    out = os.path.join(d, "res")
    r = subprocess.run([sys.executable, TRACE, "--log", p, "--tree", d, "--out", out],
                       capture_output=True, text=True)
    if r.returncode != 0:
        print("FAIL %s: rc=%d\n%s\n%s" % (name, r.returncode, r.stdout, r.stderr))
        return None
    row = {}
    with open(out + ".seams9.tsv") as f:
        hdr = f.readline().rstrip("\n").split("\t")
        for line in f:
            v = line.rstrip("\n").split("\t")
            if v[0] == "1-2":
                row = dict(zip(hdr, v))
    return row

T2 = [[1, 0, 0, 100.0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]
def cand(m, y=1, x=2):
    return ["[SFS-336]\tCAND\t7\t%d\t%d\t0.5000\t100\t0.000000\t0.000000\t0\t100\t1.0000\t" % (y, x) + mat12(m)]
surv = lambda m: ["[SFS-336]\tSURV\t7\t1\t2\t0.5000\t0.000000\t" + mat12(m)]

cases = {}
cases["A"] = case("A", cand(T2) + surv(T2))
cases["B"] = case("B", cand(T2) + ["[SFS-336]\tDROP\tpair_axis\t7\t1\t2\t0.500000"])
cases["C"] = case("C", cand(T2) + ["[SFS-336]\tMERGE\tpair\t9\t7\t1\t2\t0.40000\t0.50000"])

# non-identity axis: T_axis_2 = rotz(90, (10,20,0)); graph pose G = rotz(-30, (50,0,0));
# saved F2 = G * T_axis_2 ; GT_2 = inv(F1) F2 = F2 (F1 = T_axis_1 = I)
A2 = rotz(90, (10, 20, 0))
G = rotz(-30, (50, 0, 0))
F2 = mul(G, A2)
nodes = ["[SFS-336]\tNODE\t1\t0\t0\t0\tinit\t1\t1\t" + mat12(I),
         "[SFS-336]\tNODE\t1\t0\t0\t0\tinit\t2\t0\t" + mat12(G),
         "[SFS-336]\tNODE\t1\t0\t0\t0\tveto\t1\t1\t" + mat12(I),
         "[SFS-336]\tNODE\t1\t0\t0\t0\tveto\t2\t0\t" + mat12(G)]
cases["AXIS_D"] = case("AXIS_D", cand(mul(A2, I))[:0] + [], axis2=A2, gt2=F2, final2=F2,
                       node_lines=nodes)
# same geometry, and the candidate supplied in the u->v direction: must invert back to E(1,2)
cand_uv = mul(mul(inv(mul(A2, I)), G), I)  # x=1,y=2 candidate A = T_al_1^-1 T_al_2^-1? build below
# A(x=1,y=2) = inv(T_al_2) T_al_1 = inv(G)
cases["AXIS_A"] = case("AXIS_A", cand(inv(G), y=2, x=1) +
                       ["[SFS-336]\tSURV\t7\t2\t1\t0.5000\t0.000000\t" + mat12(inv(G))],
                       axis2=A2, gt2=F2, final2=F2)

for k, row in cases.items():
    if row is None:
        continue
    print("%-8s class=%s deciding=%s rot=%s trans=%s first_phase=%s note=%s" % (
        k, row["class_"], row["deciding"], row["rot_err"], row["trans_err"],
        row["first_phase"], row["note"]))

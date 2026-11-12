import math
hdr=None; rows=[]
for ln in open("diag_juglet.log",errors="ignore"):
    f=ln.rstrip("\n").split("\t")
    if f[0]=="DISP" and f[1]=="idx": hdr=f; continue
    if f[0]=="DISP" and hdr: rows.append(dict(zip(hdr[1:],f[1:])))
print("DISP rows", len(rows))
sent=[r for r in rows if abs(float(r["score"])-11.0)<1e-9]
print("score==11 rows", len(sent))
flip_surv=[r for r in rows if r["erased_by"]=="survived" and float(r["rot_err_deg"])>90 and float(r["axis_deg"])<math.degrees(0.436)]
print("survived, >90 deg from GT, folded axis < 25.0 deg:", len(flip_surv))
ag=[float(r["axis_raw_deg"]) for r in rows if r["erased_by"]=="axis_gate"]
print("axis_gate n", len(ag), "axis_raw range %.2f - %.2f" % (min(ag), max(ag)))
print("any axis_gate row with axis_raw>160 or <20:", [ (r["idx"], r["axis_raw_deg"]) for r in rows if r["erased_by"]=="axis_gate" and (float(r["axis_raw_deg"])>160 or float(r["axis_raw_deg"])<20)])
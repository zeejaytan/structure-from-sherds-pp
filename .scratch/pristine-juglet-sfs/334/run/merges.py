
hdr=None; rows={}
for ln in open("diag_juglet.log",errors="ignore"):
    f=ln.rstrip("\n").split("\t")
    if f[0]=="DISP" and f[1]=="idx": hdr=f; continue
    if f[0]=="DISP" and hdr: rows[int(f[1])]=dict(zip(hdr[1:],f[1:]))
print("idx  y x gt sz ncor   rot   score  erased_by     -> keeper(rot,score)")
for i in sorted(rows):
    r=rows[i]; k=r.get("merged_into","-")
    if k in ("-","",None): continue
    kk=rows.get(int(k))
    print(f"{i:4d} {r['y']:>2s} {r['x']:>2s} {r['gt_edge']:>3s} {r['size']:>3s} {float(r['ncor']):4.0f} {float(r['rot_err_deg']):6.2f} {float(r['score']):7.4f}  {r['erased_by']:>12s} -> {k:>4s} ({float(kk['rot_err_deg']):.2f}, {float(kk['score']):.4f})" if kk else i)

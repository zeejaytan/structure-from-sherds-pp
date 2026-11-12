
import json
base = "C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330/"
d = json.load(open(base+"state_verdicts.json"))
for k in ("x_t326_mor1_out","x_t326_k3r1_out"):
    print("==",k, d[k]['sherd'], d[k]['edges'])
    for b in d[k]['bad'][:3]:
        print("  ", b if not isinstance(b,dict) else {kk:(round(vv,3) if isinstance(vv,float) else vv) for kk,vv in b.items()})
    print("   n bad:", len(d[k]['bad']))

#!/usr/bin/env python3
"""analyze332.py - ticket 332 report tables. Reads the run dir only; prints measurements, no verdict."""
import json, os, sys, math
import numpy as np

RUN = sys.argv[1] if len(sys.argv) > 1 else '.'
DS = '/data/gpfs/projects/punim2657/sfs_preprocessing/Dataset'
NINE = [(1,2),(1,4),(1,8),(1,9),(2,5),(2,9),(3,5),(6,7),(7,9)]
EXP_SPARSE = [59,32,44,31,43,45,54,40,22]
EXP_DENSE  = [123,68,94,66,90,94,112,84,49]
JUG = [('gt18','gt','18'),('gt09','gt','09'),('gt09w12','gt','09'),
       ('ds18','ds','18'),('ds09','ds','09'),('ds09w12','ds','09')]

def tsv(p):
    rows = []
    if not os.path.isfile(p):
        return rows
    lines = [l.rstrip('\n') for l in open(p, errors='replace') if l.strip()]
    hdr = lines[0].split('\t')
    for l in lines[1:]:
        rows.append(dict(zip(hdr, l.split('\t'))))
    return rows

def axis(d):
    v = np.atleast_2d(np.loadtxt(d))[0]
    return v[:3], v[3:6] / np.linalg.norm(v[3:6])

print('== Part A: breakline dot counts (arm 18 = shipped 1.8 mm walk, arm 09 = dense 0.9 mm) ==')
print('   expected ticket-68 numbers: sparse 59/32/44/31/43/45/54/40/22, dense 123/68/94/66/90/94/112/84/49')
for cell in ('gt', 'ds'):
    rows = tsv(os.path.join(RUN, 'out', cell, 'pieces69.tsv'))
    if not rows:
        print('  cell %s: no pieces69.tsv yet' % cell); continue
    print('  cell %s' % cell)
    print('    piece  bl0_18  bl1_18  bl0_09  bl1_09  ratio  vs68_dense  classify')
    for n_, r in enumerate(rows):
        def g(k):
            try: return int(r.get(k, ''))
            except Exception: return None
        a, b = g('bl0_18'), g('bl0_09')
        ratio = ('%.2f' % (b / a)) if (a and b) else '-'
        exp = EXP_DENSE[n_] if n_ < len(EXP_DENSE) else None
        cmpr = ('%+d' % (b - exp)) if (b is not None and exp is not None) else '-'
        flag = '  <-- under 50 (paper floor)' if (b is not None and b < 50) else ''
        print('    %-6s %-7s %-7s %-7s %-7s %-6s %-10s %s%s' % (
              r.get('piece','?').replace('Juglet_Piece_',''), r.get('bl0_18','-'), r.get('bl1_18','-'),
              r.get('bl0_09','-'), r.get('bl1_09','-'), ratio, cmpr, r.get('classify','-'), flag))
    print('    status: %s' % rows[0].get('status','?'))

print()
print('== Part A: shipped axes (Dataset/Axes/Juglet) vs Dataset/Juglet_GT, per piece ==')
print('    piece  axis angle deg  point offset mm')
tot = []
for i in range(1, 10):
    s = os.path.join(DS,'Axes','Juglet','Juglet_Piece_%d_Axis.xyz' % i)
    t = os.path.join(DS,'Juglet_GT','Juglet_Piece_%d_Axis.xyz' % i)
    if not (os.path.isfile(s) and os.path.isfile(t)): continue
    ps, d1 = axis(s); pg, d2 = axis(t)
    ang = math.degrees(math.acos(max(-1.0, min(1.0, float(d1 @ d2)))))
    tot.append(ang)
    print('    %-6d %-15.2f %.1f' % (i, ang, float(np.linalg.norm(ps - pg))))
if tot: print('    median %.2f deg, max %.2f deg over %d pieces' % (float(np.median(tot)), max(tot), len(tot)))

def loadjson(label):
    p = os.path.join(RUN,'score',label + '.json')
    return json.load(open(p)) if os.path.isfile(p) else None

def accrows():
    return {r.get('label'): r for r in tsv(os.path.join(RUN,'score','acc.tsv'))}

print()
print('== Parts B/C: arms x repeats ==')
acc = accrows()
labels = ['potA_r1','potA_r2'] + ['%s_r%d' % (a, r) for r in (1,2) for a,_c,_k in JUG]
print('    arm          sherd  edge   seated  right-way  T_tables  window  note')
for lab in labels:
    r = acc.get(lab, {})
    d = loadjson(lab)
    seat = d.get('seated_count') if d else None
    rw = d.get('right_way_count') if d else None
    ntab = len(d.get('pieces_with_T', [])) if d else None
    w = ''
    wf = os.path.join(RUN,'logs','window_%s.txt' % lab)
    if os.path.isfile(wf): w = open(wf).read().strip().replace('LCS window: ','') or 'absent'
    pr = os.path.join(RUN,'logs','panel_%s.row' % lab)
    note = ''
    if os.path.isfile(pr):
        f = open(pr).read().rstrip('\n').split('\t')
        if len(f) >= 8: note = 'exit=%s wall=%ss' % (f[6], f[7])
    print('    %-12s %-6s %-6s %-7s %-10s %-9s %-7s %s' % (lab, r.get('sherd','-'), r.get('edge','-'),
          seat, rw, ntab, w, note))

for r in (1, 2):
    print()
    print('== Parts B/C: the nine touching pairs, repeat %d (pass = 0.35 rad / 50 mm; strict = 5 deg / 10 mm) ==' % r)
    arms = ['%s_r%d' % (a, r) for a,_c,_k in JUG]
    js = {a: loadjson(a) for a in arms}
    print('    pair  ' + ''.join('%-26s' % a.replace('_r%d' % r, '') for a in arms))
    for (i, j) in NINE:
        cells = []
        for a in arms:
            d = js[a]
            c = '-'
            if d:
                found = None
                for e in d.get('tables', {}).get('per_edge', []):
                    if (e['i'], e['j']) == (i, j): found = e; break
                if found is None: c = 'not-a-graph-edge'
                else:
                    c = ('PASS' if found['pass'] else 'no') + ('/strict' if found.get('strict') else '')
                    c += ' %.0fdeg %.0fmm' % (found['deg'], found['mm'])
            cells.append(c)
        print('    %-6s' % ('%d-%d' % (i, j)) + ''.join('%-26s' % c for c in cells))
    tall = []
    for a in arms:
        d = js[a]
        if not d: tall.append('%s:-' % a.replace('_r%d' % r,'')); continue
        got = [e for e in d.get('tables', {}).get('per_edge', []) if (e['i'], e['j']) in NINE and e['pass']]
        st = [e for e in got if e.get('strict')]
        tall.append('%s:%d/9(strict %d)' % (a.replace('_r%d' % r,''), len(got), len(st)))
    print('    tally  ' + '  '.join(tall))

print()
print('== window banner per arm ==')
ld = os.path.join(RUN,'logs')
for f in sorted(os.listdir(ld)) if os.path.isdir(ld) else []:
    if f.startswith('window_') and f.endswith('.txt'):
        print('    %-14s %s' % (f[7:-4], open(os.path.join(ld,f)).read().strip() or '<absent>'))
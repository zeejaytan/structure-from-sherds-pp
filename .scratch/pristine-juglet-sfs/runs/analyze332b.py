#!/usr/bin/env python3
"""analyze332b.py - ticket 332 tables (measurements only)."""
import json, os, sys
import numpy as np

RUN = sys.argv[1] if len(sys.argv) > 1 else '.'
DS = '/data/gpfs/projects/punim2657/sfs_preprocessing/Dataset'
NINE = [(1,2),(1,4),(1,8),(1,9),(2,5),(2,9),(3,5),(6,7),(7,9)]
CELLS = {'gt18':'gt','gt09':'gt','gt09w12':'gt','ds18':'ds','ds09':'ds','ds09w12':'ds'}
ARMS = ['gt18','gt09','gt09w12','ds18','ds09','ds09w12']

def tsv(p):
    rows = []
    if not os.path.isfile(p): return rows
    L = [l.rstrip('\n') for l in open(p, errors='replace') if l.strip()]
    hdr = L[0].split('\t')
    for l in L[1:]: rows.append(dict(zip(hdr, l.split('\t'))))
    return rows

print('== Part A: breakline dot counts per cell ==')
print('   piece  gt18  gt09  ratio   ds18  ds09  ratio')
for cell_rows_g, cell_rows_d in ((tsv(RUN+'/out/gt/pieces69.tsv'), tsv(RUN+'/out/ds/pieces69.tsv')),):
    for k, r in enumerate(cell_rows_g):
        def gi(row, key):
            try: return int(row.get(key,''))
            except Exception: return None
        d = cell_rows_d[k] if k < len(cell_rows_d) else {}
        a,b = gi(r,'bl0_18'), gi(r,'bl0_09')
        c,e = gi(d,'bl0_18'), gi(d,'bl0_09')
        print('   %-6s %-5s %-5s %-7s %-5s %-5s %s' % (r.get('piece','?').replace('Juglet_Piece_',''), a, b,
              ('%.2f'%(b/a)) if (a and b) else '-', c, e, ('%.2f'%(e/c)) if (c and e) else '-'))

def js(lab):
    p = os.path.join(RUN,'score',lab+'.json')
    return json.load(open(p)) if os.path.isfile(p) else None

def banner(lab):
    d = CELLS.get(lab.split('_')[0])
    if not d: return ''
    p = os.path.join(RUN,'work',d,'logs','binary_%s.log' % lab)
    if not os.path.isfile(p): return ''
    return open(p, errors='replace').readline().strip()

def wall(lab):
    p = os.path.join(RUN,'logs','panel_%s.row' % lab)
    if not os.path.isfile(p): return ''
    f = open(p).read().rstrip('\n').split('\t')
    return 'exit=%s wall=%ss' % (f[6], f[7]) if len(f) >= 8 else ''

print()
print('== Parts B/C: three arms x two repeats (sherd and edge are the CountResult reproduction,')
print('   the same metric as the 2/9 1/18 cured-tree baseline; seat = score58 strict own-place bar) ==')
print('   %-12s %-7s %-7s %-6s %-6s %-8s %-6s %s' % ('arm','sherd','edge','seat','rway','pairs9','window','run'))
rows = []
for lab in [a+'_r%d' % r for r in (1,2) for a in ARMS]:
    d = js(lab)
    if not d:
        print('   %-12s (missing)' % lab); continue
    cr = d['countresult']; B = banner(lab)
    got = [e for e in d['tables']['per_edge'] if (e['i'],e['j']) in NINE and e['pass']]
    print('   %-12s %-7s %-7s %-6s %-6s %-8s %-6s %s' % (lab,
          '%d/9' % cr['k_sherd'], '%d/18' % (cr['k_directed_pass']//2),
          d['seated_count'], d['right_way_count'], '%d/9' % len(got),
          B.replace('LCS window: ','') or '?', wall(lab)))

for r in (1,2):
    print()
    print('== nine touching pairs, repeat %d (deg / mm; P=pass 0.35rad/50mm, S=strict 5deg/10mm) ==' % r)
    labs = ['%s_r%d' % (a, r) for a in ARMS]
    print('   pair   ' + ''.join('%-24s' % a for a in ARMS))
    for (i,j) in NINE:
        out = []
        for a in ARMS:
            d = js('%s_r%d' % (a, r))
            if not d: out.append('-'); continue
            f = [e for e in d['tables']['per_edge'] if (e['i'],e['j'])==(i,j)]
            if not f: out.append('n/a'); continue
            e = f[0]
            out.append('%s %6.1f/%6.1f' % ('P' if e['pass'] else ('S' if e['strict'] else '.'), e['deg'], e['mm']))
        print('   %-6s ' % ('%d-%d' % (i,j)) + ''.join('%-24s' % o for o in out))

print()
print('== Pot A neutrality (window unset, POT_A include root) ==')
for r in (1,2):
    lab = 'potA_r%d' % r
    p = os.path.join(RUN,'score','acc_%s.row' % lab)
    acc = tsv(os.path.join(RUN,'score','acc.tsv'))
    hit = [x for x in acc if x.get('label') == lab]
    print('   %-9s %s  %s' % (lab, (hit[0].get('sherd','-')+' '+hit[0].get('edge','-')+' edges') if hit else 'no acc row',
          wall(lab)))
    jj = js(lab)
    if jj: print('              seats %s right-way %s' % (jj.get('seated_count'), jj.get('right_way_count')))

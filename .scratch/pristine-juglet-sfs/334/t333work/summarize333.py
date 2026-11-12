#!/usr/bin/env python3
"""summarize333.py - ticket 333: one table per arm from the run's own files.

Reads, for each label:
  out/<label>/1. Acc.txt            -> the assembler's two "Sherd accuracy : k / N" lines
  logs/binary_<label>.log           -> Total number, Total number pruned, [T333-PAIR], [T333-CLUSTER]
  score/<label>.json                -> score58's independent recomputation (countresult, per_edge)
  logs/banner_<label>.txt           -> the inlier gate banner
Prints JSON plus a markdown table. A non-GT admitted pair is a [T333-PAIR] pair not among the nine
GT pairs. Upside-down is measured on GT pairs only (per_edge deg > 90): a non-GT pair has no GT
relative pose to be upside down against.
"""
import argparse, json, os, re, sys

GT_PAIRS = {(1, 2), (1, 4), (1, 8), (1, 9), (2, 5), (2, 9), (3, 5), (6, 7), (7, 9)}


def norm(i, j):
    return (min(i, j), max(i, j))


def parse_binary(path):
    d = {'total_before_pruning': None, 'admitted': None, 'pairs': [], 'clusters': [],
         'graphs': None, 'accuracy_lines': [], 'sherd': None, 'edge': None}
    if not os.path.exists(path):
        return d
    for ln in open(path, errors='ignore'):
        if ln.startswith('Total number : '):
            d['total_before_pruning'] = int(ln.split(':')[1].strip())
        elif ln.startswith('Total number pruned : '):
            d['admitted'] = int(ln.split(':')[1].strip())
        elif ln.startswith('[T333-PAIRS] admitted '):
            d['admitted'] = int(ln.split()[-1])
        elif ln.startswith('[T333-PAIR] '):
            tok = ln.split()
            rec = {'i': int(tok[1]), 'j': int(tok[2])}
            for t in tok[3:]:
                if '=' in t:
                    key, val = t.split('=', 1)
                    try:
                        rec[key] = float(val)
                    except ValueError:
                        rec[key] = val
            d['pairs'].append(rec)
        elif ln.startswith('[T333-CLUSTERS] graphs '):
            d['graphs'] = int(ln.split()[-1])
        elif ln.startswith('[T333-CLUSTER] '):
            rest = ln.split(':', 1)[1].split()
            d['clusters'].append([int(x) for x in rest])
        elif 'Sherd accuracy' in ln or 'Edge accuracy' in ln:
            m = re.search(r'(\d+)\s*/\s*(\d+)', ln)
            if m:
                val = (int(m.group(1)), int(m.group(2)))
                if 'Edge accuracy' in ln:
                    d['edge'] = val
                else:
                    d['sherd'] = val
    return d


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--run', required=True)
    ap.add_argument('--labels', required=True)
    ap.add_argument('--out', required=True)
    a = ap.parse_args()

    rows = []
    for label in a.labels.split(','):
        b = parse_binary(os.path.join(a.run, 'logs', 'binary_%s.log' % label))
        jf = os.path.join(a.run, 'score', '%s.json' % label)
        js = json.load(open(jf)) if os.path.exists(jf) else {}
        banner = ''
        bf = os.path.join(a.run, 'logs', 'banner_%s.txt' % label)
        if os.path.exists(bf):
            banner = open(bf).read().strip()
        cr = js.get('countresult', {})
        per_edge = js.get('tables', {}).get('per_edge', [])
        up = sorted({norm(int(e['i']), int(e['j'])) for e in per_edge
                     if 'deg' in e and e['deg'] > 90.0
                     and norm(int(e['i']), int(e['j'])) in GT_PAIRS})
        admitted = {norm(r['i'], r['j']) for r in b['pairs']}
        non_gt = sorted(admitted - GT_PAIRS)
        missing_gt = sorted(GT_PAIRS - admitted)
        rows.append({
            'label': label,
            'banner': banner,
            'accuracy_lines': b['accuracy_lines'],
            'sherd': b['sherd'],
            'edge': b['edge'],
            'total_before_pruning': b['total_before_pruning'],
            'admitted_log': b['admitted'],
            'admitted': len(admitted),
            'admitted_non_gt': [list(p) for p in non_gt],
            'gt_not_admitted': [list(p) for p in missing_gt],
            'graphs': b['graphs'],
            'clusters': b['clusters'],
            'countresult': cr,
            'upside_down_gt_pairs': [list(p) for p in up],
            'per_edge': per_edge,
        })

    with open(a.out, 'w') as fh:
        json.dump(rows, fh, indent=1)
    print('%-9s %-18s %-7s %-7s %-5s %-5s %-4s %s' % ('label', 'banner', 'sherd', 'edge', 'log', 'uniq', 'ud', 'non-GT admitted'))
    for r in rows:
        s = '%d/%d' % r['sherd'] if r['sherd'] else '?'
        e = '%d/%d' % r['edge'] if r['edge'] else '?'
        print('%-9s %-18s %-7s %-7s %-5s %-5s %-4d %s' % (r['label'], r['banner'][:18], s, e,
                                                r['admitted_log'], r['admitted'],
                                                len(r['upside_down_gt_pairs']),
                                                r['admitted_non_gt']))
    print('wrote', a.out)


if __name__ == '__main__':
    main()

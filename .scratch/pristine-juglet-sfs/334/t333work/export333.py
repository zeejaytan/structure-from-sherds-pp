#!/usr/bin/env python3
"""export333.py - ticket 333: one consolidated OBJ + one two-view PNG per assembled run.

Reads the assembler's own output directory (<Result>), which holds one transformed OBJ per placed
piece (<date>_Top_1_OBJ_<k>.obj, written by class/visualize.cpp:504-532 SaveResult) plus
T_<i>to_<i>.txt. Concatenates every placed OBJ into a single OBJ with one group per piece (so the
conservator can orbit the whole assembly in visual-qa), and draws a two-view scatter snapshot so the
run can be eyeballed without loading meshes. The worker does not judge the pictures.
"""
import argparse, glob, os, re


def read_obj(path):
    vs, fs = [], []
    with open(path, 'r', errors='ignore') as fh:
        for ln in fh:
            if ln.startswith('v '):
                p = ln.split()
                vs.append((float(p[1]), float(p[2]), float(p[3])))
            elif ln.startswith('f '):
                p = ln.split()[1:]
                idx = []
                for tok in p:
                    idx.append(int(tok.split('/')[0]))
                fs.append(idx)
    return vs, fs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--result', required=True)
    ap.add_argument('--label', required=True)
    ap.add_argument('--obj', required=True)
    ap.add_argument('--png', required=True)
    ap.add_argument('--title', default='')
    a = ap.parse_args()

    files = sorted(glob.glob(os.path.join(a.result, '*_Top_1_OBJ_*.obj')))
    pieces = []
    for f in files:
        m = re.search(r'_Top_1_OBJ_(\d+)\.obj$', f)
        k = int(m.group(1)) if m else -1
        vs, fs = read_obj(f)
        pieces.append((k, f, vs, fs))
    pieces.sort()

    nv = 0
    with open(a.obj, 'w') as out:
        out.write('# ticket 333 consolidated assembly: %s\n' % a.label)
        for k, f, vs, fs in pieces:
            out.write('g piece_%d\n' % k)
            for v in vs:
                out.write('v %.6f %.6f %.6f\n' % v)
            for face in fs:
                out.write('f %s\n' % ' '.join(str(nv + i) for i in face))
            nv += len(vs)
    print('OBJ %s  pieces=%d verts=%d' % (a.obj, len(pieces), nv))

    import numpy as np
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt

    fig = plt.figure(figsize=(13, 6.2))
    views = [(18, -60), (18, 30)]
    allv = np.array([v for _, _, vs, _ in pieces for v in vs]) if pieces else np.zeros((1, 3))
    lo, hi = allv.min(0), allv.max(0)
    mid = (lo + hi) / 2.0
    rad = float((hi - lo).max()) / 2.0 * 1.05 + 1e-9
    cmap = plt.get_cmap('tab10')
    for i, (el, az) in enumerate(views):
        ax = fig.add_subplot(1, 2, i + 1, projection='3d')
        for j, (k, f, vs, fs) in enumerate(pieces):
            if not vs:
                continue
            v = np.array(vs)
            step = max(1, len(v) // 20000)
            v = v[::step]
            ax.scatter(v[:, 0], v[:, 1], v[:, 2], s=0.35, color=cmap(j % 10), linewidths=0,
                       label='piece %d' % k)
        ax.set_xlim(mid[0] - rad, mid[0] + rad)
        ax.set_ylim(mid[1] - rad, mid[1] + rad)
        ax.set_zlim(mid[2] - rad, mid[2] + rad)
        ax.set_box_aspect((1, 1, 1))
        ax.view_init(elev=el, azim=az)
        ax.set_xlabel('x mm'); ax.set_ylabel('y mm')
        ax.set_zlabel('z mm')
        ax.set_title('%s  el=%d az=%d' % (a.title or a.label, el, az), fontsize=9)
        if i == 0 and pieces:
            ax.legend(markerscale=8, fontsize=6, loc='upper left')
    fig.suptitle('%s : %d placed pieces, %d vertices (ticket 333)' % (a.label, len(pieces), nv),
                 fontsize=10)
    fig.tight_layout()
    fig.savefig(a.png, dpi=130)
    print('PNG %s' % a.png)


if __name__ == '__main__':
    main()

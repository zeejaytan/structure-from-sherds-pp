#!/usr/bin/env python3
"""stage333.py - ticket 333: build the 3x juglet_large input tree.

Sources (ticket 333, Part A; nothing is re-run):
  Breaklines/  t331 large run outlarge_jug/geom/Juglet_Piece_N.Bl0  (only Bl0; the assembler never
               reads Bl1) -> Juglet_Piece_N_Breakline_0.pcd verbatim.
  Surfaces/    same geom dir, Juglet_Piece_N.S0 / .S1 ASCII PLY -> bare 6-number .xyz
               (ReadXYZ at class/data_structure.cpp:781-843 cannot parse a PLY header).
  Axes/        Dataset/Juglet_large/Juglet_Piece_N_Axis.xyz (GT axes, already 3x).
  Ground Truth/Dataset/Juglet_large/Juglet_Piece_N_T.txt and Juglet_simple_graph.txt.
  Mesh/        Dataset/Juglet_large/Piece0N.obj -> Juglet_Piece_N_Mesh.obj (1..9).
  pota/        hardlinks of the POT_A files, so a control run never writes into the shared bundle.

Also writes <pins>/tree333.md5: the md5 of every file in the juglet tree, so the tree this ticket is
measured on can be named. Mirrors the layout of t332's work/gt/trees/base18/juglet.
"""
import argparse, hashlib, os, shutil

GRAPH_MD5 = 'ccae01dd9c897e92beb0d507baa45cea'


def md5(p):
    h = hashlib.md5()
    with open(p, 'rb') as fh:
        for b in iter(lambda: fh.read(1 << 20), b''):
            h.update(b)
    return h.hexdigest()


def read_bl0(path):
    lines = open(path, 'r', errors='ignore').read().splitlines()
    nseg = int(lines[1].split()[1])
    dots = []
    started = False
    for ln in lines:
        if not started:
            if ln.startswith('DATA'):
                started = True
            continue
        p = ln.split()
        if len(p) >= 7:
            dots.append(p[:7])
    return nseg, dots


def ply_to_xyz(src, dst, want=6):
    n = None
    hdr_end = None
    with open(src, 'r', errors='ignore') as fh:
        lines = fh.read().splitlines()
    for i, ln in enumerate(lines):
        s = ln.split()
        if len(s) >= 3 and s[0] == 'element' and s[1] == 'vertex':
            n = int(s[2])
        if ln.strip() == 'end_header':
            hdr_end = i
            break
    if n is None or hdr_end is None:
        raise SystemExit('not an ascii PLY: ' + src)
    body = lines[hdr_end + 1: hdr_end + 1 + n]
    with open(dst, 'w') as fh:
        for ln in body:
            p = ln.split()
            fh.write(' '.join(p[:want]) + '\n')
    return n


def link(s, t):
    if not os.path.exists(s):
        raise SystemExit('missing input: ' + s)
    if os.path.exists(t):
        os.remove(t)
    try:
        os.link(s, t)
    except OSError:
        shutil.copyfile(s, t)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--lanes', required=True)     # t331 outlarge_jug/geom
    ap.add_argument('--dataset', required=True)   # Dataset/Juglet_large
    ap.add_argument('--pota', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--pins', required=True)
    a = ap.parse_args()

    subdirs = ('Breaklines', 'Surfaces', 'Mesh', 'Axes', 'Ground Truth')
    J = os.path.join(a.out, 'juglet')
    P = os.path.join(a.out, 'pota')
    for T in (J, P):
        for d in subdirs:
            os.makedirs(os.path.join(T, d), exist_ok=True)
    os.makedirs(a.pins, exist_ok=True)

    man = []
    for i in range(1, 10):
        stem = 'Juglet_Piece_%d' % i
        src_bl0 = os.path.join(a.lanes, stem + '.Bl0')
        nseg, dots = read_bl0(src_bl0)
        dst = os.path.join(J, 'Breaklines', stem + '_Breakline_0.pcd')
        shutil.copyfile(src_bl0, dst)
        man.append('BL0 %s nseg=%d dots=%d md5=%s' % (stem, nseg, len(dots), md5(src_bl0)))
        for k in ('0', '1'):
            src = os.path.join(a.lanes, '%s.S%s' % (stem, k))
            nv = ply_to_xyz(src, os.path.join(J, 'Surfaces', stem + '_Surface_%s.xyz' % k))
            man.append('SURF %s S%s verts=%d md5=%s' % (stem, k, nv, md5(src)))
        pairs = (
            (os.path.join(a.dataset, 'Piece%02d.obj' % i), os.path.join(J, 'Mesh', stem + '_Mesh.obj')),
            (os.path.join(a.dataset, stem + '_Axis.xyz'), os.path.join(J, 'Axes', stem + '_Axis.xyz')),
            (os.path.join(a.dataset, stem + '_T.txt'), os.path.join(J, 'Ground Truth', stem + '_T.txt')),
        )
        for s, t in pairs:
            if not os.path.exists(s):
                raise SystemExit('missing input: ' + s)
            shutil.copyfile(s, t)
            man.append('COPY %s md5=%s' % (os.path.basename(t), md5(s)))
    g = os.path.join(a.dataset, 'Juglet_simple_graph.txt')
    got = md5(g)
    if got != GRAPH_MD5:
        raise SystemExit('graph md5 %s != expected %s' % (got, GRAPH_MD5))
    shutil.copyfile(g, os.path.join(J, 'Ground Truth', 'Juglet_simple_graph.txt'))
    man.append('GTGRAPH md5=%s' % got)

    for i in range(1, 9):
        d2 = '%02d' % i
        for rel, name in (
            ('Breaklines', 'Pot_A_Piece_%s_Breakline_0.pcd' % d2),
            ('Mesh', 'Pot_A_Piece_%s_Mesh.obj' % d2),
            ('Axes', 'Pot_A_Piece_%s_Axis.xyz' % d2),
            ('Surfaces', 'Pot_A_Piece_%s_Surface_0.xyz' % d2),
            ('Surfaces', 'Pot_A_Piece_%s_Surface_1.xyz' % d2),
        ):
            link(os.path.join(a.pota, rel, name), os.path.join(P, rel, name))
        link(os.path.join(a.pota, 'Ground Truth', 'Pot_A_Piece_%d_T.txt' % i),
             os.path.join(P, 'Ground Truth', 'Pot_A_Piece_%d_T.txt' % i))
    link(os.path.join(a.pota, 'Ground Truth', 'Pot_A_simple_graph.txt'),
         os.path.join(P, 'Ground Truth', 'Pot_A_simple_graph.txt'))

    with open(os.path.join(a.out, 'stage_manifest.txt'), 'w') as fh:
        fh.write('\n'.join(man) + '\n')

    pins = []
    for root, dirs, files in os.walk(J):
        dirs.sort()
        for f in sorted(files):
            p = os.path.join(root, f)
            pins.append('%s  %s' % (md5(p), os.path.relpath(p, a.out)))
    with open(os.path.join(a.pins, 'tree333.md5'), 'w') as fh:
        fh.write('\n'.join(pins) + '\n')

    print('\n'.join(man))
    print('PINS %d files -> %s' % (len(pins), os.path.join(a.pins, 'tree333.md5')))
    print('STAGE OK', a.out)


if __name__ == '__main__':
    main()

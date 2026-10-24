"""Identify the single passing directed edge (job 31898200 run).

Mirrors CountResult (data_structure.cpp:1000-1066): for each directed
edge the METHOD's graph proposes on a GT edge, compare GT-relative vs
method-relative transforms with isSimilarTrans(rad 0.35, t 50mm).
Method graph edges parsed from assembly_data.txt; final transforms from
results T_XtoX.txt; GT from the staged Ground Truth dir.
"""
import math
import os
import re
import sys

import numpy as np

RES = "/data/gpfs/projects/punim2657/sfs_main/results_2026_10_05_1915/"
ASM = "/data/gpfs/projects/punim2657/sfs_main/"
GTD = ("/data/gpfs/projects/punim2657/sfs_main/pota_e2e_fresh/SfS_pp/"
       "Ground Truth/")


def load_T(path):
    return np.loadtxt(path).reshape(4, 4)


def is_similar(GT_T, T, rad=0.35, t_thr=50.0):
    E = np.linalg.inv(GT_T) @ T
    ang = math.degrees(math.acos(min(1.0, max(-1.0,
                                             (np.trace(E[:3, :3]) - 1) / 2))))
    tra = float(np.linalg.norm(E[:3, 3]))
    return ang, tra, (ang <= math.degrees(rad) and tra <= t_thr)


def main():
    GTg = np.loadtxt(os.path.join(GTD, "Pot_A_simple_graph.txt"),
                     dtype=int)
    Tres, have = {}, set()
    for i in range(8):
        p = os.path.join(RES, f"Result/T_{i}to_{i}.txt")
        if os.path.exists(p):
            Tres[i] = load_T(p)
            have.add(i)
    if len(have) < 8:
        print(f"note: missing final transforms for pieces "
              f"{sorted(set(range(8)) - have)} (never placed)")
    GTt = [load_T(os.path.join(GTD, f"Pot_A_Piece_{i + 1}_T.txt"))
           for i in range(8)]
    # All 30 directed GT edges (CountResult additionally requires the
    # method to propose the edge; k=1 total means a single pass below is
    # on a proposed edge).
    for i in range(8):
        for j in range(8):
            if not GTg[i, j] or i not in have or j not in have:
                continue
            GT_T = np.linalg.inv(GTt[i]) @ GTt[j]
            T = np.linalg.inv(Tres[i]) @ Tres[j]
            ang, tra, ok = is_similar(GT_T, T)
            mark = "  <-- PASS" if ok else ""
            print(f"directed {i + 1}->{j + 1}: rot {ang:6.1f}deg "
                  f"trans {tra:8.1f}mm{mark}")


if __name__ == "__main__":
    main()

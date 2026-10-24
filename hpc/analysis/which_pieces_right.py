"""Which pieces landed right in the Registration-150 run (job 31898200).

Compares final per-piece transforms (results T_XtoX.txt) against GT.
Frame note: if T_XtoX maps scan->final and GT maps scan->vessel, then
inv(GT)@T measures final-vs-GT discrepancy per piece. Rotation error +
translation error reported; correctness judged generously (<=5deg, <=5mm).
"""
import math
import os

import numpy as np

RES = "/data/gpfs/projects/punim2657/sfs_main/results_2026_10_01_2139/Result/"
GTD = ("/data/gpfs/projects/punim2657/sfs_preprocessing/"
       "NURBS_Dataset_20251103/SfS_pp/Ground Truth/")


def main():
    for i in range(8):
        T = np.loadtxt(os.path.join(RES, f"T_{i}to_{i}.txt")).reshape(4, 4)
        G = np.loadtxt(os.path.join(GTD, f"Pot_A_Piece_{i + 1}_T.txt"))
        G = G.reshape(4, 4)
        E = np.linalg.inv(G) @ T
        ang = math.degrees(math.acos(min(1.0, max(-1.0,
                                                 (np.trace(E[:3, :3]) - 1) / 2))))
        tra = float(np.linalg.norm(E[:3, 3]))
        ok = "OK" if (ang <= 5.0 and tra <= 5.0) else "off"
        print(f"piece {i + 1}: rot-err {ang:7.1f}deg "
              f"trans-err {tra:9.1f}mm  [{ok}]")


if __name__ == "__main__":
    main()

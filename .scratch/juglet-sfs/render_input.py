"""Render Juglet SFS++ inputs (scan poses, mm) + GT assembly check."""
import h5py
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

BASE = r"C:\PR\structure-from-sherds-pp\artifacts\juglet_input"
K = 70.7098803591493
S = 15.61135  # global scale norm -> gt frame

hn = h5py.File(f"{BASE}/juglet_norm.hdf5", "r")
hg = h5py.File(r"C:\PR\tora\artifacts\juglet_gt.hdf5", "r")
cols = plt.cm.tab10.colors

fig = plt.figure(figsize=(14, 7))
for k, (title, get) in enumerate([
    ("inputs: 9 sherds, scan-table poses (mm)",
     lambda i: np.asarray(hn[f"juglet_norm/Juglet-000/pieces/{i}/vertices"][:]) * K * S),
    ("answer key: conservator reassembly (mm)",
     lambda i: np.asarray(hg[f"juglet_gt/Juglet-000/pieces/{i}/vertices"][:]) * K),
]):
    ax = fig.add_subplot(1, 2, k + 1, projection="3d")
    for i in range(9):
        v = get(i)
        w = v[::max(1, len(v) // 1500)]
        ax.scatter(w[:, 0], w[:, 1], w[:, 2], s=1.0, c=[cols[i]],
                   label=f"P{i + 1:02d}")
    ax.view_init(elev=15, azim=-60)
    ax.set_title(title)
    ax.set_xlabel("x mm")
    ax.set_ylabel("y mm")
    ax.set_box_aspect((1, 1, 1))
fig.axes[1].legend(markerscale=5, fontsize=7, loc="upper left")
plt.tight_layout()
plt.savefig(f"{BASE}/preview.png", dpi=110)
print("saved preview.png")

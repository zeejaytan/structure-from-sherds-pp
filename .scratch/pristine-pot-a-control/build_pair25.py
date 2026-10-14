"""Pristine Pot A control (01-pristine-pot-a-run): build the Needs-eye pair.

Pair 2-5 (the S1-canonical control pair): GT-placed meshes (left) vs
pristine-attempt-placed meshes (right, T_1to_1/T_4to_4 from job 32983523).
Original meshes are laptop-local and md5-verified identical to the cluster
inputs, so no heavy fetch. Prints a seam-proximity measurement with the pair.
"""
import numpy as np
import trimesh

ART = "C:/PR/structure-from-sherds-pp/artifacts"
MESH = ART + "/pot_a_mesh_all/Pot_A_Piece_%02d_Mesh.obj"
GT = ART + "/pot_a_gt/Pot_A_Piece_%d_T.txt"
OUT = ART + "/visual-qa/pristine_pota_25/pair25_%s.ply"

# Attempt transforms (job 32983523 Result_snapshot, T_1to_1 / T_4to_4).
T_ATT = {
    2: np.array([[-0.218341, -0.898418, 0.381015, -114.667],
                 [0.898292, -0.33759, -0.281255, 82.4776],
                 [0.381312, 0.280853, 0.880751, -355.726],
                 [0, 0, 0, 1]]),
    5: np.array([[-0.593364, -0.211099, 0.77676, -339.093],
                 [-0.338707, 0.940887, -0.00303362, -38.0756],
                 [-0.730203, -0.264894, -0.629789, 239.886],
                 [0, 0, 0, 1]]),
}
TAN = np.array([210, 180, 140, 255], dtype=np.uint8)
TEAL = np.array([0, 150, 150, 255], dtype=np.uint8)


def placed(piece, T):
    m = trimesh.load(MESH % piece, force="mesh")
    m.apply_transform(T)
    return np.asarray(m.vertices)


def colored(verts, color, faces_owner_shapes):
    return verts, color


import os
os.makedirs(ART + "/visual-qa/pristine_pota_25", exist_ok=True)

for side, get_T in (("gt", lambda p: np.loadtxt(GT % p).reshape(4, 4)),
                    ("attempt", lambda p: T_ATT[p])):
    meshes = []
    for piece, col in ((2, TAN), (5, TEAL)):
        m = trimesh.load(MESH % piece, force="mesh")
        m.apply_transform(get_T(piece))
        m.visual.vertex_colors = np.tile(col, (len(m.vertices), 1))
        meshes.append(m)
    combo = trimesh.util.concatenate(meshes)
    combo.export(OUT % side)
    from scipy.spatial import cKDTree
    v2 = np.asarray(meshes[0].vertices)[::10]
    v5 = np.asarray(meshes[1].vertices)[::10]
    d, _ = cKDTree(v5).query(v2)
    print(f"{side}: wrote {OUT % side}; "
          f"piece2->piece5 min-vertex gap: median {np.median(d):.2f} mm, "
          f"min {d.min():.2f} mm; "
          f"share vertices <2mm: {(d < 2.0).sum()}/{len(d)}")

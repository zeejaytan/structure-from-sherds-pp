# 01: Juglet-side data block + bind

**What to build:** the untouched binary reads the Juglet bundle through a
`/Dataset`-based data block, exactly the way the Pot A control reads its samples —
so the Spartan run serves real Juglet curves with zero algorithm changes.

**Answers:** S3

**Blocked by:** None (can start immediately).

**Status:** results-in — block added, bind recorded; intent write-back lands via tickets 02/03

- [x] `class/data_path.h` gains a JUGLET_ORIG-style block mirroring POT_A_ORIG (:1610–1707): SHARD_NUMBER 9, `/Dataset`-relative base, 9 breakline_0 paths, 9 meshes, 9 axes, 18 surfaces, 9 T + simple graph (names mirror the existing JUGLET block :1498–1604, base swapped) — added as `#ifdef JUGLET_ORIG` at `class/data_path.h:1715–1820`, `JUGLET_ORIG_BASE "/Dataset/SfS_pp/"` (:1719)
- [x] Existing JUGLET block (:1498–1604) and POT_A_ORIG block untouched; active macro unchanged — verified: JUGLET `#endif` still :1604, POT_A_ORIG `#endif` still :1707, `#define POT_A` still active (:10)
- [x] Juglet-side `--bind` host path recorded here (host dir containing the `SfS_pp` bundle root) — host dir: `/data/gpfs/projects/punim2657/sfs_preprocessing/Juglet_Dataset_20260916`; verbatim flag for ticket 02: `--bind "/data/gpfs/projects/punim2657/sfs_preprocessing/Juglet_Dataset_20260916/SfS_pp:/Dataset/SfS_pp"` (mirrors the Pot A O2 pattern, spec line 25)
- [x] Gate probe paths resolve against the new block (dry check: filenames match bundle listing) — stems identical to JUGLET block :1498–1604; single-digit `Juglet_Piece_%d` matches `artifacts/juglet_run1/gate_probe_b0.py:15-16,95-96` and the 9/9/9/18/9 bundle census (spec lines 16-17)

## Comments

- 2026-10-15 (juglet-block-worker): Block added with `edit` anchored on the unique `#endif` + `#ifdef POT_B` seam; no other lines in `class/data_path.h` touched. `JUGLET_ORIG` left inactive (no `#define`), so the compiled default is byte-identical to before. Ticket 02 selects it with `-DJUGLET_ORIG` (and deselects `POT_A`) at build time.

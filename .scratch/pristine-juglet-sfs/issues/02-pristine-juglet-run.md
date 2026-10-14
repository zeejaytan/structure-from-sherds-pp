# 02: Pristine Juglet assembly run

**What to build:** a Spartan run of the untouched binary (upstream @9195c01 + Rank 2
shim, container recipe as-is) over the Juglet bundle via the ticket-01 block, at the
paper's k=5/b=3 — producing a join count the thesis can cite either way.

**Answers:** S3

**Blocked by:** 01 (data block + bind must land first).

**Status:** results-in — gate probe + two E2E runs recorded; scoring/verdict stays in ticket 03

- [x] Gate probe `gate_probe_b0.py juglet` recorded first (pass count at GT)
- [x] Assembly submitted (shim build job + run job IDs recorded below with sacct State/ExitCode via `scripts/slurm_poll.sh`)
- [x] Run config recorded: upstream SHA, k/b, bind flag, data block name
- [x] Raw output (SA/EA, seated pairs, scores) pasted under Comments; no verdict here (verdict lives in S3 via ticket 03)

## Comments (worker report 2026-10-15, juglet-run-worker)

RESULT: gate probe STRICT 0/18 at GT; E2E SA 2/9 + EA 0/18, reproduced
identically by two runs. A 0-result honestly reported counts as a valid
answer; the numbers below are raw, no verdict (ticket 03).

- Gate probe: `python artifacts/juglet_run1/gate_probe_b0.py juglet`
  (laptop-side; login-node python3 lacks scipy) → `STRICT pass rate at GT
  (_0-only): 0/18`. Only pairs with any strict inliers even with
  abs-normals: 2-9 (abs=6, min_d 1.68) and 6-7 (abs=19, min_d 0.92);
  signed-normal strict raw=0 on all 18 pairs.
- Isolation: tree `/data/gpfs/projects/punim2657/sfs_main/archive_2025_sfs_modified_src`.
  `rev-parse HEAD` = `9195c0167e2077a564bd7c91156063252feeb506`,
  `status --porcelain` empty (verified in-job both runs). SHA matches the
  pinned SHA, no discrepancy.
- Binding: host `/data/gpfs/projects/punim2657/sfs_preprocessing/Juglet_Dataset_20260916/SfS_pp`
  → container `/Dataset/SfS_pp` (verbatim ticket-01 flag). Census: 9×
  `Juglet_Piece_*_Breakline_0.pcd` (+ _1 + .ply/.xyz sidecars), Mesh 9/9,
  Axes 9/9, Surfaces 89 Juglet files, Ground Truth 10 (9 T + graph).
  Surface_F: 0 files — inert per ticket 01 (graceful load). Never
  `sfs_legacy_temp`.
- Purity: zero `class/` changes (archive tree untouched, verified clean
  in-job). Declared INFRA: JUGLET_ORIG block (`class/data_path.h:1715–1820`
  @ 978b96c, md5 `401195eae5a41579563fc237a9843298` — verified in-job)
  appended to a /tmp COPY of the pristine `data_path.h` with upstream
  `#define POT_A` renamed off in that copy only; `/Dataset` bind; sbatch
  `sfs_main/run_pristine_juglet.sbatch` (untracked, Spartan-only, staged
  via scp — NOT committed, per this ticket's edit boundary). Shim
  `hpc/tools/main_pristine_batch.cpp` reused as-is, md5
  `fb26e8ab49e9c00f7e8d02de44f6eea5` both sides. SIF
  `sfs_main/sfs_pristine_assembly.sif` reused as-is per spec (810M, Oct 14;
  no separate build job — shim compiled out-of-source inside each run job,
  pristine objects 100%). No POT_A/JUGLET macro branches exist in
  pristine sources outside `data_path.h` (grep empty), so the header swap
  is data-selection only. `sfspreproc.sif` never used.
- E2E job 32993790 (COMPLETED 0:0 per sacct `COMPLETED|0:0|2026-10-15T01:38:22`,
  ~6 min wall): `Total number : 1410`, `Total number pruned : 41`,
  `Sherd accuracy : 2 / 9 (22.2222%)`, `Edge accuracy : 0 / 18 (2.77778%)`,
  binary exit 0. SaveAcc failed (`File system error`) — the Juglet bundle
  has no pre-existing `Result/` dir (Pot A bundle had legacy files) and the
  binary never mkdirs; snapshot empty. No `co-first` tie line.
- E2E job 32994273 (COMPLETED 0:0 per sacct `COMPLETED|0:0|2026-10-15T01:46:09`,
  same script + `mkdir -p /Dataset/SfS_pp/Result` pre-binary): identical
  scores (SA 2/9, EA 0/18 — deterministic reproduction), full snapshot in
  `sfs_main/pristine_juglet_32994273/Result_snapshot/`: `1. Acc.txt`
  (`Time : 382643 Sec`, `2 / 9`, 2nd line `0 / 18` under mislabelled
  `Sherd accuracy` — same upstream quirk as Pot A), 9×
  `2026_10_15_1_46_Top_1_Edgeline_*.xyz`, 9× `Top_1_OBJ_*.obj`, `T_0to_0`
  … `T_8to_8.txt`. Host bundle left as found (`Result/` removed post-run;
  bundle root verified: Axes Breaklines Ground Truth Mesh _quarantine
  Surfaces only).
- Config: upstream SHA 9195c01, k=5/b=3 (shim defaults), bind flag per
  ticket 01, block JUGLET_ORIG (SHARD_NUMBER 9, binary arg 9).
- Seated pairs: the binary prints only totals — which 2 sherds seated is
  NOT in the log; attribution from `Result_snapshot/T_*.txt` vs GT is left
  for ticket 03 (paths above).
- Process notes for lead: (1) fork checkout
  `sfs_main/sfspreproc-docker` @ eb6c389 — `git pull --ff-only` ABORTED
  (`Please move or remove them before you merge`; untracked live files
  incl. pristine sbatches). Left untouched per boundary; run used
  `origin/main` (= 978b96c, fetch had succeeded) content via `git show`
  without modifying the checkout. Ticket 03 worker will hit the same wall
  if it needs a pull. (2) `scripts/slurm_poll.sh` unrunnable from this
  laptop (WSL bash rejects `set -o pipefail`); polled with an equivalent
  pwsh ssh loop (120 s cadence) + `sacct -j <id>
  --format=State,ExitCode,End -n -P`; sacct lines above.

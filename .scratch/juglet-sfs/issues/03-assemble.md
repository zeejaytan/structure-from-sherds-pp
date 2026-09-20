# 03: Assemble the Juglet headless and render the verdict

**What to build:** an SFS++ assembly run over the 9 Juglet pieces and a
witnessed verdict both a conservator and S1 can use: connections found
(count + which pairs), and a render of what the machine claims joins.

**Answers:** S1

**Blocked by:** 02

**Status:** ready-for-agent

## Comments

- 2026-09-20: research before acting (all verified in code, not assumed):
  format checks (axes/breaklines/surfaces/GT all parse; missing
  Surface_F + Breakline_1 inert: graceful loads, guarded use,
  empty-safe tree/correspondence); no behavior branches on dataset
  defines; container :/workspace bind verified for sfspreproc.sif;
  binary is Hierarchy-Clear (Tray script's -headless suffix stale).
  Deviations from the ticket text: REAL GT paths (derived ticket 01,
  after that line was written -- scored assembly serves S1);
  run_juglet.sbatch at repo root (matches 10 existing run scripts).
  JUGLET block committed (absolute bundle paths, SHARD_NUMBER 9);
  active define POT_A -> JUGLET. Submitted as 30829350 (6 h:
  fresh build + 9-piece run); ps1-watched.

- 2026-09-20: job 30829350/30829426/30829480 tripped two script bugs
  (surface count 18 vs 9; space in "Ground Truth" dir), then the
  :/workspace bind proved shadowed in sfspreproc.sif too (site GPFS
  mount; my first check used a file present in both trees -- lesson:
  verify with a file unique to one side). Fixed to /data binds.
  Resubmitted as 30829588; COMPLETED.

- 2026-09-20 VERDICT (job 30829588, evidence in
  `structure-from-sherds-pp/artifacts/juglet_run1/`):
  best assembly = 7 singleton sherds (1,3,4,5,6,7,8), score 0 --
  ZERO of 18 joins proposed; sherds 2,9 not placed. Per-pair truth:
  4/18 true mates had feature matches (1-6: 29 matches; 4-5, 3-7,
  7-9: 3-4); ICP gave inliers on five true pairs (best 1-6: 84,
  7-9: 63, 4-5: 22) yet none merged; 14/18 true mates had ZERO
  feature matches. The printed 100% counters are VACUOUS (GT-init
  scoring: unplaced pieces keep GT values; placed score as
  GT x ~identity -- T_ files byte-match GT). Claim: genuine method
  outcome on this object (inputs mm-verified, bundle gated), NOT a
  ruler artefact this time; the 100% is disbelieved with mechanism.
  Weight: ONE object, ONE run; S2 gate applies (handmade/handled,
  out of axial scope) -- a scope data point, not a capability verdict.
  Open mechanism: why inlier pairs never merge (no-merge-root loop;
  LCS curve-length thresholds suspect, unverified).

## Needs-eye (staged, awaiting witness)

- Pair `juglet_sfs` staged in the viewer (serve
  `visual-qa/viewer/view_juglet.py`, port 8080, Check for new pairs):
  LEFT = sherds 1+6 at correct relative placement (conservator GT,
  min gap 3.6 mm -- eyeballed GT, not touching); RIGHT = same two
  sherds as the machine left them (scan poses, 13.7 mm apart, no join
  proposed). Piece 1 tan, piece 6 teal. Tool auto-locked overlay off
  (487 mm frame offset, side-by-side only) -- correct for this pair.
  Desc at `visual-qa/viewer/pairs/juglet_sfs.json` (local-only: the
  `visual-qa/` tree is umbrella-gitignored); geometry under
  `structure-from-sherds-pp/artifacts/juglet_sfs/` (gitignored).
- Witness needed: conservator look + note (does the LEFT pair read as
  the true 1-6 join? does RIGHT read as unjoined?), then agent reply
  per the issue-tracker convention.

**Needs-eye:** viewer bundle TBD at staging time (stage under
`visual-qa/viewer/pairs/juglet_sfs.json`)

- [ ] JUGLET block in `class/data_path.h` (SHARD_NUMBER 9, no GT paths needed —
      missing GT degrades to identity, accuracy section reports 0/0)
- [ ] Headless build + `run_juglet.sbatch` from `sfs_main/`; poll to completion
- [ ] Connection count reported; if still zero, radius-vs-cloud units printed
- [ ] At least one proposed join rendered at a view that shows the break faces
- [ ] Result written back into `intent/S1-does-it-connect-anything-real.md`

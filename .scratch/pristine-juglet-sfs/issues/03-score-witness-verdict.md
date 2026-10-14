# 03: Score, witness render, verdict into S3

**What to build:** the run's numbers scored against the honest denominator, the
reassembly looked at (not just counted), and S3 updated — so the loop closes in
`intent/`, not in a log.

**Answers:** S3

**Blocked by:** 02 (needs the run output).

**Needs-eye:** `visual-qa/preview/manifest-juglet-pristine.json` (stage the pair with the visual-qa-helper; mesh/point-cloud pair, never a bare picture)

**Status:** results-in — attributed + staged by juglet-score-worker 2026-10-15; S3 verdict + board rebuild with lead

- [x] SA/EA scored vs honest 0/10 denominator (and nominal /18 noted); V-B tolerances τR=20° τT=50mm
- [ ] Render staged at a view resolving the seam scale; witnessed look + conservator note recorded below (numbers alone do not close this ticket)
- [x] S3 `Where it stands` + Done-when boxes updated with date, job IDs, scores, and the verdict (untouched agrees / diverges)
- [ ] Board rebuilt (`python scripts/build_board.py`) in the same commit

## Comments (lead note 2026-10-15)

Verdict written in S3 (untouched agrees, honest 0/10); render staged with frame_ok
but the witnessed look is still pending, so boxes 2 and 4 stay open and this ticket
stays results-in. Desc wording reviewed against the 2026-09-28 rule (title is a
look-for question, methods are a few words each) and kept.

## Comments (worker report 2026-10-15, juglet-score-worker)

RESULT: binary SA 2/9 + EA 0/18 reproduced exactly from the snapshot; the 2
"seated" sherds are pieces 1 + 6 via ONE directed relative edge (6→1) on the
NON-contact pair 1-6 — no touching edge recovered, honest EA 0/10. No verdict
here (S3 is lead-owned).

- Recipe pointer: sbatch `sfs_main/run_pristine_juglet.sbatch` (untracked,
  Spartan-only, staged via scp per ticket 02 — NOT committed). Jobs 32993790
  (no-Result-dir run, SaveAcc failed) + 32994273 (full snapshot; identical
  SA 2/9, EA 0/18). Gate probe STRICT 0/18 at GT (ticket 02). Full run
  config + sacct lines in ticket 02 Comments.
- Attribution method: snapshot `sfs_main/pristine_juglet_32994273/Result_snapshot/`
  `T_ito_i.txt` (piece N ↔ T_(N-1); order verified against the JUGLET_ORIG
  block `class/data_path.h:1722–1730`) vs bundle GT. Small samples fetched to
  `artifacts/pristine_juglet_32994273/` (9× T, Acc.txt as `Acc.txt`, 9× GT T +
  graph). Laptop GT md5-identical to Spartan bundle GT (all 10 files, e.g.
  Piece_1 `0d936379b768cdfb19a9906fa3e36a02`). Scorer replica
  `artifacts/pristine_juglet_32994273/score_seats.py` mirrors
  `CountResult` (`class/data_structure.cpp:993–1066`, thresholds
  rad 0.35 / t 50.0 at :1000).
- KEY MECHANISM (why 2/9 with 0/18): the binary's SA is NOT absolute pose —
  a sherd counts as seated if ≥1 proposed relative edge passes
  (`:1043–1059`). EA prints integer-divided k/2 (:1063–1065), so k=1 directed
  pass shows as `0 / 18 (2.77778%)` — 1/36. Acc.txt line 2 mislabels the edge
  line `Sherd accuracy` (same upstream quirk as Pot A, cf. ticket 02).
- Per-seat ABSOLUTE errors (est T vs GT, τR=20° τT=50mm) — 0/9 seated:
  p1 173.42°/633.7mm; p2 151.73°/771.4mm; p3 47.57°/351.5mm;
  p4 137.49°/768.0mm; p5 35.37°/324.4mm; p6 174.56°/973.5mm;
  p7 124.85°/869.1mm; p8 88.54°/95.5mm; p9 131.71°/691.1mm.
- Directed relative check over all 36 (18 pairs × 2 dirs): EXACTLY one pass —
  6→1 rot 12.72° trans 39.29mm; reverse 1→6 rot 12.72° trans 103.71mm FAILS.
  Replica gives k=1 → SA endpoints {1,6} = 2/9, EA 0/18 — matches the binary
  exactly, so attribution is exact (no proposed-graph ambiguity: only one
  geometric pass exists). 1-6 is a NON-contact (S3:38–39), so honest EA 0/10;
  nominal EA 0/18. No touching pair passes in either direction (nearest:
  1-4 fwd 38.9°/100.7mm — over on both).
- Chance reading (evidence, not verdict): 0/9 absolute seats (best rot 35.4°,
  best trans 95.5mm); single marginal pass on a non-touching pair, one
  direction only, at 12.7°/39.3mm inside a (20°, 50mm) gate over 36 directed
  trials; gate probe 0/18 strict at GT (pipeline cannot recognise true mates).
- Staged witness pair (mesh/point-cloud, no bare picture):
  manifest `visual-qa/preview/manifest-juglet-pristine.json` (Needs-eye bundle
  path); desc `visual-qa/viewer/pairs/pristine_juglet.json` (DRAFT wording —
  lead owns the desc, please review); build
  `structure-from-sherds-pp/artifacts/visual-qa/pristine_juglet/build_whole.py`
  (Pot A whole-vessel recipe: bundle Mesh/ + snapshot T + GT T, one global
  Kabsch alignment 157.30°/|t| 9.84mm, raw attempt kept as whole_attempt.ply).
  Left = 9 meshes at GT (coherent juglet, verified by eye in a colored
  render before staging); right = 9 meshes at attempt poses (scrambled
  cluster). `stage_pair.py` gates passed: 120000-face previews, median edge
  0.441mm both sides, coincidence median 2.13mm (frame_ok true, no banner),
  scale sidecars written. Extra fetches for staging (all small):
  `artifacts/juglet_mesh/` 9× Mesh OBJ (3.9MB total) + snapshot edgelines
  1 + 6 (~11KB each, the two nominally-seated sherds).
- Boundaries kept: diverged fork checkout `sfs_main/sfspreproc-docker` @
  eb6c389 untouched (no pull); `intent/` untouched — S3 write-up, verdict,
  and board rebuild are with the lead. No commit (session commits per
  finished ticket).

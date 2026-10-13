# 11: Fork-vs-upstream audit — assembly (validate every change, document it)

**Answers:** S1

**Blocked by:** nothing — read-only until the doc says otherwise

**Status:** resolved 2026-10-13 (doc re-audit landed; follow-ups queued: Huber revert ticket, GraduatedDistScale dead-helper, UTF-8 glyph restore, 05 correction addendum)

**Needs-eye:** none — diff classification, no geometry claim.

## Why this ticket exists

Same debt as preprocessing ticket 19, assembly side: our
`structure-from-sherds-pp` fork layers cluster-lineage work (alternate
optimizers, env-gated branches, tuning comments, `main_headless_correct`
itself — which does NOT exist upstream) plus our tickets (dropped-shard
logging, SG smoother arm, base-flag completion, Surface_F removal,
IcpFine guard, paper-config values, T0 dump, profile stddev, dataset
flips). No document maps change → paper relation. Ticket 05 audited
paper-vs-code but never fork-vs-upstream (provenance: which hunks are
ours vs inherited). This ticket is that map.

## Baselines (pin these; if upstream moves, re-pin and say so)

- Upstream: `upstream/main` @ `9195c016` (fetch ref, 2026-10-01).
- Ours: `origin/HEAD` at time of audit (record the SHA in the doc).
- Note: `main_headless_correct.cpp` (±1197-line diff) has NO upstream
  counterpart — attribute it whole (cluster lineage, presumably) via
  `git log`, then classify OUR hunks within it. Same for
  `multi_hypothesis_optimizer*`, `puzzlefusion_*`, `two_phase_*`,
  `hub_guided_*`, physics files: lineage-or-ours first, paper relation
  second.

## Method (in this order — attribution before classification)

1. `git diff upstream/main HEAD -- class/ main*.cpp CMakeLists.txt`,
   grouped by file/hunk. Attribute every hunk: (a) cluster lineage,
   (b) our ticketed change (cite ticket), (c) unattributed (findings).
2. New-file inventory (SG arm already tracked; optimizer files; hpc
   scripts/sbatch; ggce tests): one line each.
3. Classify every OUR hunk: PAPER-MATCH / DEVIATION-measured /
   ROBUSTNESS-no-behavior / INFRA / UNATTRIBUTED (same scheme as
   preprocessing ticket 19 — the two docs must use identical labels).
4. Cross-check against ticket 05's verdict table + paper-silent inventory:
   any hunk with paper behavior that 05 doesn't know about is a finding;
   any "ours" behavior 05 listed but unattributed here gets its ticket.

## Deliverable (the "then document")

`FORK_VS_UPSTREAM.md` at repo root: pinned SHAs, per-file hunk table,
new-file inventory, unattributed list (empty or ticketed). Stands alone —
it must be readable without the tickets, with pointers in.

## Acceptance criteria

- [x] Doc exists with pinned SHAs and the per-file table
- [x] Every OUR hunk classified; every UNATTRIBUTED either ticketed or
      accepted-with-reason in the doc
- [x] Ticket-05 cross-check done (no unknown-to-05 paper-behavior hunk)
- [x] Label scheme identical to preprocessing ticket 19's doc (verified 2026-10-13: no ticket 19 exists — preprocessing side has 01–04 only — so the doc's own five-label scheme stands as recorded in §"Label scheme")
- [x] Nothing changed in code in THIS ticket (audit only)

## Audit work (2026-10-13, HEAD `26db0e5`)

Re-validation of `FORK_VS_UPSTREAM.md` (which pins ours @ `b32d819`)
against current HEAD. Read-only except this section; code untouched
(one pre-existing tracked mod is a ticket note,
`.scratch/solver-convergence/issues/02-anchor-newcomer.md` +57 — not code).

### Pinned SHAs

- Upstream `upstream/main` = `9195c0167e2077a564bd7c91156063252feeb506` —
  UNMOVED since 2026-10-01 fetch ref. No re-pin needed.
- Ours: doc pins `b32d819054b607b74ac7bb325e1601378992b37e`; audit-time
  `HEAD` = `origin/HEAD` = `26db0e51bd8dab7e56c73b11e37ed4e26f4d39d3`.
  Doc baseline is STALE by ~60 commits — re-pin proposed below.

### Scope counts (doc says 52 files +26360/−188; now 46 files +22101/−341)

Exact reconciliation: 7 scoped files deleted (ticket 10 Group A
`d1d0663`: `auto_agglomerative_assembler.{cpp,h}`,
`multi_hypothesis_optimizer_backup.cpp`,
`multi_hypothesis_optimizer_test_patch.cpp`,
`physics_based_optimizer.{cpp,h}`, `physics_integration.cpp`) +1 newly
modified (`class/axis_estimation.cpp`, thread-test hunk). 52−7+1=46. ✓
Whole-repo diff now 354 files +70383/−343 (doc: 330, +69170/−190).

### Our-era delta (b32d819..HEAD) — every hunk attributed, none UNATTRIBUTED

- PAPER-MATCH (+2/−2 net): Registration `max_iteration` 50→150 ×2
  (ticket 07 control, job 32087141, paper :240 — extends 05-deviation-1
  beyond its cited Icp site). SG-arm PAPER-MATCH + ROBUSTNESS hunks gone
  with the arm (ticket 04 `d95b5ab`, default path byte-identical).
- DEVIATION-measured (+1, first non-zero): trust-region cap
  initial 100 / max 1000 ×5 solve sites (solver-01 `5cd22a2`, A/B
  diverged 140→0, sane identical). Paper names LM (05 settled); radius
  is paper-silent solver behavior — lead to confirm label vs INFRA.
- ROBUSTNESS-no-behavior: Ticket-11 diverged-solve guards ×3 sites
  (`13ac169`); F5 drop logging in live binary (closes doc F5);
  `*** MAKEMULTI *** iter=` print; dead-code removals (WithInlier
  def+decl, FeatureComp mode-1 branch, dead `manager`, hub-DISABLED
  block — ticket 10 Group B `bfe3406`); UTF-8→`?` glyph damage in print
  literals/comments ×3 files (stdout-only, but lead should restore —
  suggests a latin-1 edit-tool pass).
- INFRA: E-0 `LogE0Residuals` + calls ×4 sites (env-gated, cauchy-scales
  01); `MakeMultiCorres` iter plumbing; LCS dump + `SFS_LCS_TWICE` gate
  (ticket 08 `e5dadfa`); 9-file deletions (inventory);
  `EIGEN_DONT_PARALLELIZE` + `num_threads=1` ×7 sites (ticket-12 thread
  test, IN-FLIGHT, unenv-gated); Huber ×3 sites (cauchy-scales Arm H,
  IN-FLIGHT — E-6 verdict landed yet still in tree, needs revert ticket);
  axis 0.610 / score 1.5 / RejectOutlier (20,0.7) (ticket-12 rows 1–3,
  upstream values — fork delta SHRINKS); MINIMUM 6 (row 6, KEEP per A/B);
  comment-only (V2a reverts, row-4 veto numbers, ANGLE 08-resolved,
  07-E3, F7 stddev fix — closes doc F7, data_path).
- `GraduatedDistScale` (E-2 schedule) is DEFINED but never called —
  dead helper, ticket-10 follow-up. F8 (`UsePreCorres` double
  `MakeSection`) still present, still no-behavior.

### Ticket-05 cross-check (05 corrected 2026-10-01 per fork audit §4)

- Doc F1 RESOLVED: code now at upstream 0.610/1.5 (12 rows 1–2, KEEP/ADOPT).
- Doc F3 RESOLVED: (20,0.7) at 3 live sites (12 row 3, ADOPT).
- Doc F2 still open → owned by 12 row 5 (next up). Doc F4 still open →
  owned by 12 row 7; 05 CORRECTIONS now records the GGCE scorer row.
- Doc F5/F7 CLOSED (verified in tree). F8 still open. F6 SPLIT:
  MINIMUM part resolved (6 restored); INLIER 3.0 still 05-unknown (minor).
- NEW G1: solver-01 trust cap — paper-behavior hunk 05 doesn't know
  (05-dev-2 covers weights/scales only). Owned by solver-01 ticket.
- NEW G2: Arm-H Huber — 05-unknown + un-reverted post-verdict.
  Owned by cauchy-scales 06.
- NEW G3 (minor): Registration-150 extends 05-dev-1's fix. Owned by 07.
- STALE-05: 05's silent inventory still lists mode-1 branch, hub block,
  dead manager (all removed by ticket-10 Group B) — 05 is resolved, needs
  a correction addendum (lead call). Its dead-code list was otherwise
  predictive (all 9 files now deleted).
- Label scheme: preprocessing side still has tickets 01–04 only, NO
  ticket 19 (verified 2026-10-13) — doc §6 wording stands as-is.

### Proposed FORK_VS_UPSTREAM.md patch (lead commits; NOT applied)

1. Baselines table: ours → `26db0e5…` (+ re-audit date); scope line →
   46 files +22101/−341; whole-repo → 354 files +70383/−343; upstream
   row unchanged + "verified unmoved 2026-10-13".
2. Counts line → PAPER-MATCH 15 · DEVIATION-measured 1 (pending label
   confirm) · ROBUSTNESS-no-behavior ~34 · INFRA ~20 · UNATTRIBUTED 0.
3. `filter.cpp/h` section: rewrite — SG arm REMOVED (04 `d95b5ab`);
   default path = Lanczos + Gaussian(2,k10), byte-identical.
4. `feature_matching.cpp`: WithInlier + mode-1 branch gone (10-B);
   MergeSimPair comment → V2a; axis 0.610 / score 1.5 now UPSTREAM values
   (12 rows 1–2); row-4 veto comment with numbers; F1 → resolved.
5. `reconstruction.cpp`: RejectOutlier (20,0.7) UPSTREAM (12 row 3, F3
   resolved); Registration 150 ×2 (07 control); guards ×3 (11);
   Huber ×3 (Arm H in-flight, G2); trust cap ×5 (solver-01, G1);
   LogE0 + GraduatedDistScale-dead note; MakeMultiCorres iter plumbing.
6. `reconstruction.h`: MINIMUM 6 (12 row 6); ANGLE comment 08-resolved.
7. `main_headless_correct.cpp`: 1295 lines; +F5 logging (F5 closed),
   −dead manager, −hub block (10-B), +LCS dump/TWICE (08), +includes.
8. `CMakeLists.txt`: now HAS our hunk — `EIGEN_DONT_PARALLELIZE`
   (thread test, in-flight). Section changes from "cluster only".
9. New-file inventory: mark REMOVED — backup, test_patch,
   auto_agglomerative, physics ×3, root surface_overlap_detection (10-A);
   `axis_estimation.cpp` enters scope (modified); add sbatch inventory —
   run_e0/e2/e3/e4/e5/e6, run_row2/3/4, run_solver01/02,
   run_ticket08_lcs_ab (one line each); add `.scratch/cauchy-scales`,
   `.scratch/solver-convergence`, juglet-sfs 08–12.
10. §4: F1/F3/F5/F7 closed; F2→12-row-5; F4→12-row-7; F6 split; F8 open;
    new G1/G2/G3; 05-stale-inventory note.
11. §7 acceptance mapping: re-count; code-unchanged holds (this ticket
    wrote no code).

# Fork vs upstream — assembly (`structure-from-sherds-pp`)

Audit of every change our fork layers on top of upstream, and each change's
relation to the paper. Stands alone; pointers to tickets at the end.

Paper: `C:\PR\papers\text\sfspp-2502.13986v1.md` (cited as "paper :LINE").
Ticket-05 audit: `.scratch/paper-compliance/issues/05-matching-methodology-audit.md`
(cited as "05"). This doc answers assembly ticket 11
(`.scratch/paper-compliance/issues/11-fork-vs-upstream-assembly.md`, `Answers: S1`).

## Pinned baselines

| Baseline | SHA |
|---|---|
| Upstream (`upstream/main`, fetch ref 2026-10-01) | `9195c0167e2077a564bd7c91156063252feeb506` |
| Ours (`origin/HEAD` = `HEAD` at audit time, tree clean) | `b32d819054b607b74ac7bb325e1601378992b37e` |
| Cluster-merge baseline (see §1) | `3de8a5e` |

Scope of the hunk tables: `git diff upstream/main HEAD -- class/ main*.cpp
CMakeLists.txt` (52 files, +26360/−188). Whole-repo diff is 330 files,
+69170/−190 — the rest is `hpc/`, `.scratch/`, `intent/`, root run scripts,
covered as inventory only (§5).

## Label scheme

Identical to the scheme ticket 11 prescribes (shared with the preprocessing
audit). Labels apply to **our** hunks (Sep 2026 – Oct 2026 tickets).
Cluster-lineage hunks get attribution + a paper-relation note, not a label.

- **PAPER-MATCH** — hunk implements a paper-stated value or structure, with
  paper line cited. Our paper-config arm (`3ff4f8d`) and measured fixes live here.
- **DEVIATION-measured** — hunk knowingly deviates from the paper, with owning
  ticket + measured numbers. (Count in this audit: **zero** — open deviations
  are owned by tickets 06/07/08/09 as future work; none of our hunks in this
  diff knowingly deviates.)
- **ROBUSTNESS-no-behavior** — hardening, diagnostics, dead-code removal, or
  fail-loud guards that do not change matching behavior on valid input
  (stdout-only prints; env gates whose defaults preserve current values;
  guards that fire only on corrupt/contract-violating input).
- **INFRA** — build, dataset selection, test/measure harness, env-gated
  experiment arms that are off by default (zero behavior change when unset).
- **UNATTRIBUTED** — our-era hunk with no ticket and no accepted reason.
  (Count in this audit: **zero**, see §6.)

Counts: PAPER-MATCH 14 · DEVIATION-measured 0 · ROBUSTNESS-no-behavior 28 ·
INFRA 11 · UNATTRIBUTED 0. Every our-era hunk is claimed below.

## 1. Lineage: where "ours vs inherited" splits

Cluster work arrived as **two sibling imports** off upstream `9195c01`, merged
2025-11-15 in `3de8a5e` ("Merge remote changes with local SFS++ enhancements",
parents `[869e696 0c71c49]`):

- `d2445d8` (2025-11-05, "Add Structure-from-Sherds++ modifications and
  fixes"): `main_headless.cpp` (346 lines) + hunks in tracked files
  (`data_structure.cpp`, `feature_matching.cpp`, `ranking_system.cpp`,
  `reconstruction.cpp/h`, `data_path.h`, `main.cpp`, `CMakeLists.txt` 30 lines).
- `869e696` (2025-11-15, "Add SFS++ research enhancements and pottery-aware
  validation"): all optimizer new files, `main_headless_correct.cpp`
  (1132 lines), pottery validators, the `CMakeLists.txt` rewrite, plus hunks
  in tracked files.

Attribution method: `git diff upstream/main 3de8a5e` = cluster lineage;
`git diff 3de8a5e HEAD` = our era. Everything in the fork is in exactly one
of the two (both cluster lines are ancestors of `HEAD` via the merge).

Our era = Jul–Oct 2026 tickets: SG smoother arm (ticket 04), Juglet
diagnosis instruments (Sep 2026 "Instrument/Print …" commits), dropped-shard
logging (`9c21efe`, via SfSpp_preprocessing ticket 09), Surface_F removal +
`NO_BASE_INFO` completion (`c0a7f1b`, assembly-02), dataset flips
(JUGLET / POT_A_ORIG / `/Dataset/` revert), paper-config arm (`3ff4f8d`,
2026-10-01), oracle/TAXIS/dump harness (ticket 06 Test 2), profile dump T0
(ticket 10), profile stddev fix (ticket 06 fix 1, `e049413`).

## 2. Files that exist upstream (modified in fork)

### `class/data_path.h` — our 229/3 on cluster 5/1

- Cluster: `// Dataset selection` + `TRAY_000` comments; `path` pointed at a
  host-absolute NURBS bundle (paper-relation: none — environment path).
- Ours, INFRA (4 hunks): POT_A selection comment (E2E 2026-10-01);
  `/Dataset/` revert + comment (in-container bind; the absolute path loaded
  zero files in-container); `JUGLET` 9-sherd block; `POT_A_ORIG` oracle-control
  block. The `JUGLET` insert replaced only the decorative `ICCV Pottery B`
  header line; the `#ifdef POT_B` block itself is intact (`:1708`).
  Dataset selection is config, not algorithm — no paper relation.

### `class/data_structure.cpp` — our 5/2 on cluster 82/9

- Cluster (paper-relation notes): `original_axis_norm_` plumbing through all
  four copy/move operators + `ReadAxis` preserve + axis-load prints (axis
  handling our files depend on; paper-silent); standard-PCD fallback parser
  (accepts header-only PCDs as single segment — input robustness, paper-silent);
  NURBS curvature column load into `feature_[6]` (dead unless a 7-column PCD
  arrives; paper-silent).
- Ours, ROBUSTNESS-no-behavior (1 hunk): `CountResult` honest gate (Juglet
  ticket 04, 2026-09-21) — an edge scores only if BOTH answer key and method
  propose it (`run 30829588: 18/18` false-perfect before). Scoring honesty;
  matching path untouched.

### `class/data_structure.h` — cluster only (92/13, no our hunks)

All cluster: `original_axis_norm_` / `original_axis_height_` fields; explicit
`Trans` + `LCSIndex` copy/assign ("corruption fixes", Nov 2025); upstream
`operator=(int)` disabled (commented, "corrupts fields during std::sort").
Paper-relation: copy semantics, paper-silent. Correctness hardening we inherit.

### `class/feature_matching.cpp` — our 61/16 on cluster 284/30

Our hunks:

| Hunk | Label | Ticket / evidence |
|---|---|---|
| `ProfileChecking` corrupt-input guards (non-finite reject, empty reject, absurd z-range reject) | ROBUSTNESS-no-behavior | 06 guard; fires only on input that previously hit UB (`.back()` on empty) or vacuous pass (bin-count overflow) |
| `ProfileChecking` stddev rule + `[start,end]` off-by-one fix | PAPER-MATCH | 06 fix 1; paper :550 (stddev of orthogonal distances, 7 mm); measured 20/20 recorded failures flipped, 0 new false passes. Fit stays OLS — deviation owned by 06, still open |
| `MergeSimPair` `0.175`→`0.436` | PAPER-MATCH | paper-config; paper :294 (25°/20 mm). Companion stage question owned by 08 |
| `PairwisePruning` overlap `120.0`→`50.0` | PAPER-MATCH | paper-config; paper :548 + Table (50 mm²). Cluster's 120 carried an "ORIGINAL THRESHOLD" comment |
| `MergeOverlapTest` `100`→`50` | PAPER-MATCH | paper-config; same paper lines |
| `RegistrationPruning` axis gate env (`SFS_AXIS_ANGLE_MAX`, default 0.436) + dead base-flag locals removed | ROBUSTNESS-no-behavior | Juglet ticket 05 (gate) + assembly-02 (removal); default path byte-identical |
| `*** PRUNEENTRY ***` print (`db7ddd7`), `*** REGOUT ***` prints ×2 (`4dd938a`) | ROBUSTNESS-no-behavior | Sep-2026 Juglet instruments; stdout only |

Cluster base (paper-relation notes, 05 cross-check in §4): `CheckOpposingNormals`
definition (its call stays DISABLED — 05 knows); `*** PROFILE VALIDATION ***
Starting/Checking/result` prints; `CountInlier` adaptive segment contribution
+ acos clamps + inlier-debug prints (**F2**); pottery-aware `CountInlier`
overload + `isPotteryValidationEnabled` branch (**F3** part); `Q_size`
"Restored to ORIGINAL" comments (values identical to upstream — comment-only);
basket gates axis `0.436`→`1.571` / score `1.5`→`4.0` (**F1**);
`RegistrationPruning` IntersectionDetector + `overlap \|\| has_intersection`
(05 knows the ratios); hub-scoring lines; `RejectOutlier` tightenings
(**F3**); `RejectOutlier(cor_in, 20, 0.65)` REMOVED-out line in `Icp`
(05 lists the (20,0.65) value).

### `class/feature_matching.h` — cluster only (12/1)

Cluster: pottery/intersection/hub includes; `Clustering` default `20`→`5`
("REDUCED … to prevent false clustering"). Callers still pass `Clustering(Out,
20)` (upstream lines, unchanged — 05's inventory entry describes upstream
behavior, not a fork change). 05 cross-check: header default is cluster's.

### `class/filter.cpp` / `class/filter.h` — ours only (88/3 + 6/0; no cluster hunks)

SG smoother arm (assembly ticket 04, commits `40321cc`→`5b3afdd`):

| Hunk | Label | Ticket / evidence |
|---|---|---|
| `SavitzkyGolayDiff` + `Gaussian7x2` + declarations | PAPER-MATCH | 04; implements paper Fig. 7 params (7-pt quadratic SG, Gaussian width 7 σ 2.0); A/B measured ("scores move, joins don't"), kept covered |
| `SFS_SMOOTHER` switch + `[SFS-SG]` print + if/else arm structure | ROBUSTNESS-no-behavior | 04; default (unset) path byte-identical; the if/else (not sequential) is what prevents double smoothing |

### `class/ranking_system.cpp` — our 128/32 on cluster 54/5

Our hunks:

| Hunk | Label | Ticket / evidence |
|---|---|---|
| `<cstdlib>`/`<cstdio>`/`<sstream>` includes | INFRA | build support for gates (Juglet 05) + oracle parse (06 Test 2) |
| `*** MERGEPOSE ***` print (`f097ef9`) | ROBUSTNESS-no-behavior | stdout only |
| `ExclusivelyPickEdge` base-branch (`==2`,`==3`) removal | ROBUSTNESS-no-behavior | assembly-02; function has no callers repo-wide; flag semantics impossible on current files (all writers emit 0) |
| `*** TRANSAVG ***` print (`d337ad1`) | ROBUSTNESS-no-behavior | stdout only |
| `CountPCInlier` `SFS_PC_DIST`/`SFS_PC_NORM` env gates ×2 (defaults 7.0/1.5) | ROBUSTNESS-no-behavior | Juglet ticket 05; defaults preserve behavior; tuning harness |
| Oracle-init block (`SFS_ORACLE_PAIR`/`SFS_ORACLE_M`, `Tp = Wf·M·Wm⁻¹`) | INFRA | 06 Test 2; fires only when env set; downstream runs unmodified FROM truth |
| `CheckGraphPlausibility` overlap `10`→`50` | PAPER-MATCH | paper-config; paper :548. Cluster had "REDUCED 50→10" |
| `CheckGraphPlausibility` profile `6.5/6.0`→`7.0/7.0` | PAPER-MATCH | paper-config; paper Table (7 mm bins / 7 mm). NUMBERS only — fit (OLS) + rule now stddev via 06 fix 1; the adjacent comment still says "rule stays max-deviation" — **stale, F7** |
| `SFS_PROFILE_DUMP` TSV dump block | INFRA | 10 T0; env-gated, zero behavior change when unset |
| `*** PLAUSIBILITY ***` print (`470f675`) | ROBUSTNESS-no-behavior | stdout only |
| `SingleOverlapTest` `20`→`50` | PAPER-MATCH | paper-config; paper :548. Cluster had "REDUCED 100→20" |

Cluster base: `connectivity_optimizer.h` include; 0.7/0.3
local/global combined scoring in `MakeHierarchyPriorityList` + `InlierCompare`
(**F4**); `isSingleTrans`… `isSimilarTrans 0.436`→`0.2` "BEAM SEARCH FIX"
(**F4**); `*** PROFILE DEBUG *** Checking/result/Adding/CONFIGURATION
REJECTED` prints.

### `class/ranking_system.h` — cluster only (12/5)

Cluster: `Chunk::{global_connectivity_score, combined_score}` fields
(**F4**); two whitespace-only lines; trailing-newline fix. No our hunks.

### `class/reconstruction.cpp` — our 81/9 on cluster 122/43

Our hunks:

| Hunk | Label | Ticket / evidence |
|---|---|---|
| `<cstdlib>` include | INFRA | Juglet 05 gates |
| `SFS_NORMAL_ABS` in `UnifiedPotteryValidation` fallback (default 0) | ROBUSTNESS-no-behavior | Juglet 06; default preserves raw-dot behavior; aligns with robust_icp's `abs(dot)` when set |
| `SFS_REJECT_DIST`/`SFS_REJECT_NORM` in `MakeMultiCorres` (defaults 2.0/0.85 = fork values) + `*** MAKEMULTI ***` print (`e2de280`) | ROBUSTNESS-no-behavior | Juglet 06; comment records upstream `(20, 0.7)` vs fork `(2.0, 0.85)` |
| `UsePreCorres` `*** USEPRE ***` print (`533f1e6`) + one extra `MakeSection` call | ROBUSTNESS-no-behavior | verified idempotent: `MakeSection` resizes + block-overwrites `p_A`/`p_B`, `ChangeOrder` toggles twice. Wasted work, same result |
| `Icp` `max_iteration 50`→`150` | PAPER-MATCH | paper-config; paper :240. Weights untouched (no 0.4 mapping — 07) |
| `*** ICPCOR ***` (`8b21c06`), `*** ICPITER ***`, `*** REGIN ***` prints | ROBUSTNESS-no-behavior | Juglet diagnosis; stdout only |
| `SFS_LCS_ONLY` dense-rematch gates ×2 (default dense) | ROBUSTNESS-no-behavior | Juglet 06; default preserves behavior; LCS-reuse arm is the experiment |
| `SFS_AXIS_WEIGHT` (`IcpIncGraphAxis`, default 1.0 = fork value) | ROBUSTNESS-no-behavior | Juglet 06; default preserves behavior |
| `SFS_CAUCHY_DIST`/`SFS_CAUCHY_NORM` (`IcpIncGraphAxis`, defaults 1.0/0.5 = fork values) | ROBUSTNESS-no-behavior | Juglet 06; comment records upstream 5.0/2.0 |
| `*** MERGETABLE ***` + `*** ICPINLIER ***` prints (`31715d0`) | ROBUSTNESS-no-behavior | stdout only |
| `IcpFine` empty-`COR_frac` loud-fail guard | ROBUSTNESS-no-behavior | preproc-01 removal lane; fires only where the old code solved with zero fracture residuals; zero change when frac data present |

Cluster base (paper-relation notes): `UnifiedPotteryValidation` +
`RejectOutlier→Unified` redirect + `*** LEGACY REDIRECT ***` print;
`(20,0.7)`→`(2.0,0.85)` tightenings in `MakeSingle/Merge/MultiCorres`
(**F3**); `cor.size() > 8` (was `MINIMUM_NUMBER/2`); `(10,0.7)`→`(10,0.65)`
frac gate; Cauchy `4.0/1.8/1.8/1.8` (was `3.0/1.5`, original `5.0/2.0`) — 05
deviation #2 knows these; `function_tolerance 1e-6`; `IcpIncGraphAxis`
weights `w_d 2.0/w_n 5.0/w_line 3.0/w_a 1.0`, iters 200/200, `isConverge
(0.02,0.2)`; `inlierCalculate` legacy prints; GT-debugger wiring in `Icp`.

### `class/reconstruction.h` — our 1/1 on cluster 8/2

- Cluster: `MINIMUM_NUMBER 6`→`1`, `INLIER_THRESHOLD 1.5`→`3.0`
  (**F6** — 05's silent inventory knows the MINIMUM_NUMBER value, not the
  INLIER one); `UnifiedPotteryValidation` declaration.
- Ours, PAPER-MATCH (1 hunk): `ANGLE_THRESHOLD 0.262`→`0.5236`;
  paper-config; paper :215 (30°). Stage caveat owned by ticket 08: the paper
  prunes *correspondences* at 30°; this macro gates *inlier counting*.

### `main.cpp` — our 5/2 on cluster 1/2 (NOT built — only `main_headless_correct.cpp` is in `CMakeLists.txt`)

- Cluster: `TOP_k 5` / `BRANCH_b 3` defines ("Original beam search parameters").
- Ours, ROBUSTNESS-no-behavior (2 hunks, `c0a7f1b`): 3-arg→2-arg
  `LoadSurface` (Surface_F removal) + guarded `sur_frac_` normal; `NO_BASE_INFO`
  also clears `is_sane_base_` (assembly-02 F2 — provable no-op, all writers
  emit flag 0).

### `main_headless.cpp` — NEW file, cluster base + our hunks (NOT built)

- Attribution: base = `d2445d8` (346 lines, Nov 2025 cluster); merged via
  `3de8a5e`. Our 44 lines: dropped-shard logging (`9c21efe`,
  ROBUSTNESS-no-behavior — counters + `[DROPPED]` lines + subset warning;
  drop decisions unchanged) and `is_sane_base_` clear (`c0a7f1b`,
  ROBUSTNESS-no-behavior, no-op).
- Note: the 50-point / empty-axis drop gates themselves are cluster's.
  F5 FIXED post-audit (lead, 2026-10-01): the live binary now logs
  `[DROPPED] shard i` with the point count at the <50 gate
  (`main_headless_correct.cpp`, same shape as the unbuilt main's logging).
  Empty-axis drops remain silent — same treatment owed if they ever matter.

### `main_headless_correct.cpp` — NEW file, cluster base + our hunks (THE production binary)

- Attribution whole first: NO upstream counterpart (verified: absent from
  `upstream/main` tree). Base = `869e696` (1132 lines, Nov 2025 cluster):
  headless driver, `GroundTruthDebugger`, `TOP_k 15`/`BRANCH_b 8` EXPANDED
  values, env-gated two-phase / auto-agglomerative / multi-hypothesis /
  puzzlefusion branches (default off — 05's "dead by default" verdict rests
  on `:465`/`518` gating), hub-guidance DISABLED block, dead
  `EnhancedStateManager` construction, shard-drop gates (`:172–180`,
  **F5** — logging added post-audit, see §main_headless.cpp note).
- Our hunks (68/3):

| Hunk | Label | Ticket / evidence |
|---|---|---|
| `<cstdlib>`/`<cstdio>`/`<vector>`/`<fstream>`/`<algorithm>` includes | INFRA | 06 Test 2 harness |
| `TOP_k 15`→`5`, `BRANCH_b 8`→`3` | PAPER-MATCH | paper-config; paper single-pot experiments use k=5/b=3 (:344 etc.; 05 accepted k/b as config) |
| 3-arg→2-arg `LoadSurface` | ROBUSTNESS-no-behavior | Surface_F removal (`c0a7f1b`); no Surface_F files exist |
| `NO_BASE_INFO` clears `is_sane_base_` | ROBUSTNESS-no-behavior | assembly-02 F2; no-op on current files |
| `*** TAXIS ***` per-piece axis print | INFRA | 06 Test 2; parsed offline to build oracle matrices |
| `SFS_ORACLE_INJECT` pair-prepend block | INFRA | 06 Test 2; env-gated; proposes the pair, hands no transform (neutral identity, overridden at MERGEINIT) |

### `CMakeLists.txt` — cluster only (426/19, no our hunks)

Cluster rewrite (project `Hierarchy-Clear-GGCE`): `hierarchy_core` + `ggce_lib`
static libs, TEASER++ FetchContent, `ggce_tests` CTest target, install/packaging.
Paper-relation: build only. `MAIN_SOURCES` = `main_headless_correct.cpp`
alone (05 §"Dead code" liveness claim rests on this). Our era touched nothing
here — the build still compiles every cluster optimizer file listed in §5
whether or not it executes (see F4 liveness note).

## 3. New-file inventory (one line each)

Attribution: ALL new `class/` files, both mains, and the pottery validators
landed in `869e696` (Nov 2025 cluster) except `main_headless.cpp` (`d2445d8`)
and `tests/test_ggce.*` + `test_teaser.cpp` (`35a86b2`, 2026-07-19). Our era
added no new `class/` files. "Built" = listed in `CMakeLists.txt`
`CORE_SOURCES`/`GGCE_SOURCES`/test targets.

| File | Lines | Built? | One line |
|---|---|---|---|
| `class/multi_hypothesis_optimizer.{cpp,h}` | 1896/445 | yes (core) | Alternate optimizer, env-gated (`ENABLE_MULTI_HYPOTHESIS`), dead by default; our paper-config touched 2 call-site numbers |
| `class/multi_hypothesis_optimizer_backup.cpp` | 1821 | NO | Stale copy (keeps 6.5/6.0 profile numbers); 05 dead-code removal ticket 10 |
| `class/multi_hypothesis_optimizer_test_patch.cpp` | 98 | NO | Patch draft, unbuilt; ticket 10 |
| `class/puzzlefusion_global_optimizer.{cpp,h}` | 5779/816 | yes (core) | Alternate optimizer; `Enhanced3DOverlapDetector` chain dead by default; our paper-config touched 1 call site |
| `class/two_phase_assembly.{cpp,h}` | 949/405 | yes (ggce) | Alternate assembly path, env-gated (`ENABLE_TWO_PHASE_ASSEMBLY`), dead by default |
| `class/enhanced_ranking_system.{cpp,h}` | 605/355 | yes (ggce) | `EnhancedStateManager` — constructed but unused in production path; 05 dead-by-default |
| `class/hub_guided_beam_search.{cpp,h}` | 168/73 | yes (core) | Hub scoring; application block DISABLED in production main; 05 paper-silent |
| `class/global_connectivity_engine.{cpp,h}` + `_part2.cpp` | 793/484 + 639 | yes (ggce) | GGCE scorer backing the 0.7/0.3 combined score (**F4**) |
| `class/connectivity_optimizer.{cpp,h}` | 401/187 | yes (core) | Called from live `MakeHierarchyPriorityList` — live, not dead (**F4**) |
| `class/intersection_detector.{cpp,h}` | 550/252 | yes (core) | Post-registration intersection veto, live in `RegistrationPruning` (ratios 0.20/0.35/0.15 — 05 knows) |
| `class/robust_icp.{cpp,h}` | 569/117 | yes (core) | Robust ICP variant; production path uses legacy `inlierCalculate` ("robust ICP removed" print) |
| `class/rim_base_evidence.{cpp,h}` | 662/219 | yes (core) | Rim/base evidence machinery; base half removed by decision (assembly-02) |
| `class/surface_overlap_detector.{cpp,h}` | 159/70 | yes (core) | `Enhanced3DOverlapDetector` (defaults 2.0 mm/1000.0; exercised values would be 2.0/500 — 05 liveness table) |
| `class/ground_truth_debug.h` | 138 | header-included | GT-connection tracker, live-wired in `Icp` miss path |
| `class/physics_based_optimizer.{cpp,h}` | 637/227 | NO | Unbuilt shadow; ticket 10 (05 dead code) |
| `class/physics_integration.cpp` | 220 | NO | Unbuilt shadow; ticket 10 |
| `class/full_physics_optimizer.{cpp,h}` | 750/176 | yes (core) | Compiled in, never invoked from production path |
| `class/physics_based_minimal_test.{cpp,h}` | 201/56 | yes (core) | Compiled in, never invoked from production path |
| `class/pottery_structure.{cpp,h}` | 497/121 | yes (core) | Compiled in, never invoked from production path |
| `class/auto_agglomerative_assembler.{cpp,h}` | 1199/347 | NO (env-gated ref exists, file unlisted) | Alternate assembler; ticket 10 |
| `pottery_geometric_validator.{cpp,h}` | 416/218 | NO | Full pottery validator behind `isPotteryValidationEnabled` (env `ENABLE_POTTERY_VALIDATION`, default off) |
| `pottery_geometric_validator_simple.{cpp,h}` | 6/213 | yes (core) | Thin validator (static-map impl in header); live-called only when env enables, else legacy fallback |
| `surface_overlap_detection.{cpp,h}` (root) | 209/48 | NO | Superseded standalone draft (AGENTS.md); live code is `class/surface_overlap_detector.*` |
| `main_headless.cpp` | 390 | NO | Cluster driver + our drop logging; unbuilt |
| `main_headless_correct.cpp` | 1197+ | YES (the binary) | Production driver; §2 |
| `tests/test_ggce.{cpp,h}` | 811/366 | yes (`ggce_tests`) | GGCE unit tests, CTest-registered |
| `test_teaser.cpp` | 110 | yes (`test_teaser`) | TEASER++ smoke test the build references |
| Root run/build scripts (`run_*.sh`, `run_*.sbatch`, `rerun_juglet.sbatch`, `rebuild_*`, `test_*.sbatch`, `test_ggce.sh`, `harvest_pota_taxis.sh`, `build_*.sh`, `generate_*`, `visualize_*.py`, `analyze_tray000_results.py`, `GGCE_README.md` + root research notes) | — | n/a | Our-era and cluster-era run harness + analysis notes; job scripts, not algorithm |
| `hpc/` (55 docs + 50 sbatch + 37 analysis + 26 scripts + 14 tools + 6 patches + 3 containers + README) | ~197 files | n/a | Cluster-era launch/analysis working area versioned per `hpc/README.md`; `hpc/tools` = preprocessing-side TPS/Surface_F generators (outside assembly scope) |
| `scripts/remote/` (3 files) | — | n/a | `pull_and_sbatch.sh`, `job_status.sh`, `fetch_artifacts.sh` — Slurm helpers |
| `intent/`, `.scratch/` | — | n/a | Questions + tickets, not code |

## 4. Ticket-05 cross-check (paper-behavior hunks 05 doesn't know)

Rule applied: any hunk with paper behavior (changes which pairs/poses survive
or how they score, on the LIVE path) that 05's verdict table + paper-silent
inventory do not record is a finding. Dead-by-default paths are noted, not
flagged.

- **F1 — PairwisePruning basket gates relaxed (cluster, LIVE).**
  `axis_angle 0.436`→`1.571` (25°→90°) and `lowest_score 1.5`→`4.0`, both
  "Evidence-based" comments. `PairwisePruning` is 05's live matcher path.
  05 deviation #4 covers only the `MergeSimPair` grouping gate (different
  stage). 05 does not know these two. FINDING.
- **F2 — `CountInlier` adaptive segment contribution (cluster, LIVE).**
  Segments ≥30 keep the old rule; 15–29 need ≥1 inlier; <15 need 8% density;
  plus acos clamps and a global `MINIMUM_NUMBER` zeroing. Reached via
  `inlierCalculate` on the live path. 05 deviation #7 covers `CountPCInlier`
  bands (different function). 05 does not know this one. FINDING.
- **F3 — `RejectOutlier` tightening `(20,0.7)`→`(2.0,0.85)` + `cor>8`
  (cluster, LIVE).** Three `Make*Corres` sites in `Registration`; plus the
  now-removed `Icp` inner-loop call. 05's silent inventory lists only the
  surviving `(20,0.65)` instances. FINDING.
- **F4 — Beam-loop scoring changes (cluster, LIVE).**
  `CombineChunk isSimilarTrans 0.436`→`0.2` ("BEAM SEARCH FIX") and the
  0.7/0.3 `combined_score` (via `ConnectivityOptimizer`, called from live
  `MakeHierarchyPriorityList`, compiled into `hierarchy_core` — NOT
  env-gated, unlike the dead optimizers). 05's "hub-guided scoring" silent
  line may or may not be this; 05's liveness table does not name
  `ConnectivityOptimizer`. FINDING (or, at minimum, 05's one-liner needs a
  line cite to this code).
- **F5 — Silent shard-drop gates in the PRODUCTION binary (cluster, LIVE
  input stage).** `main_headless_correct.cpp:172–180` drops axis-less and
  <50-point sherds with no log line. Our `9c21efe` logging went only to the
  unbuilt `main_headless.cpp`. A 50-point input filter with paper behavior
  (decides which sherds exist) that 05 never mentions. FINDING.
- **F6 — `INLIER_THRESHOLD 1.5`→`3.0` (cluster, LIVE) — minor gap.**
  05's silent inventory records `MINIMUM_NUMBER=1` but not the 3.0 distance.
  Noted, not commissioned.
- **F7 — Stale PAPER-CONFIG comment (ours, no behavior).**
  `ranking_system.cpp` profile call site still says "rule stays
  max-deviation (ticket 06 owns those)" after 06 fix 1 installed the stddev
  rule. Comment fix only.
- **F8 — `UsePreCorres` duplicated `MakeSection` (ours, no behavior).**
  Verified idempotent (§2 table); remove on touch.
- **Disentanglement (not findings):** four of 05's "paper-silent (ours)"
  entries are actually **upstream's**, not the fork's — LCS multipliers
  3/2.5/2.5/4 (`feature_matching.cpp:274–280`, identical upstream),
  `Clustering(Out,20)` callers (`:319/:432`, identical upstream),
  `isConverge(T,0.1,2.0)` (`reconstruction.cpp`, identical upstream), and the
  `windowsize` call structure. The fork's part is only the `Clustering`
  header default 20→5 and `MINIMUM_NUMBER` 6→1. 05's record stands (paper
  relation is correct); only the fork-attribution needs this footnote.

## 5. Unattributed list

**Empty.** Every our-era hunk carries a ticket tag in its comment
(paper-config, 06/06-Test-2, 10-T0, Juglet 04/05/06, assembly-02, preproc-01,
04/SFS-SG, SfSpp_preprocessing-09) or was traced to a named Sep 2026
Juglet-diagnosis instrument commit (`db7ddd7`, `4dd938a`, `f097ef9`,
`d337ad1`, `470f675`, `533f1e6`, `8b21c06`, `e2de280`, `31715d0`). No
`UNATTRIBUTED` label was needed. Cluster hunks without ticket tags are
expected (Nov 2025, pre-ticket era) and are attributed by commit above.

## 6. Pointers

- Spec: `.scratch/paper-compliance/issues/11-fork-vs-upstream-assembly.md`
- Paper-vs-code record: ticket 05 (same directory)
- Fix tickets owning open deviations: 06 (profile fit/stages), 07 (ICP
  weights/iterations), 08 (normal gate stage + grouping test), 09 (overlap
  unify + solve diagnosis), 10 (dead-code removal + T0 battery)
- Preprocessing-side audit (sibling scheme): SfSpp_preprocessing
  `.scratch/paper-compliance/` tickets 01–04 (no ticket 19 exists there yet;
  the label scheme used here is ticket 11's own §"Method" wording)

## 7. Acceptance mapping (ticket 11)

- Doc exists with pinned SHAs (§"Pinned baselines") and per-file tables (§2): yes
- Every OUR hunk classified (53 claimed: 14/0/28/11/0); UNATTRIBUTED empty (§5): yes
- Ticket-05 cross-check done; unknown-to-05 paper-behavior hunks flagged F1–F6: yes
- Label scheme = ticket 11's five labels: yes
- Code unchanged in this audit (doc only, uncommitted for lead review): yes

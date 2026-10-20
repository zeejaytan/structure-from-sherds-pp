# 02: Restore pristine assembly scorer (revert P22FIX/P22FIX2/C2 + fork drift)

**Goal:** assembly binary behaves exactly like pristine upstream `@9195c01` again; the only fork content that stays is Spartan-run glue. Ordered by the conservator 2026-10-20 (1b file-identical, 2b reset over pushed commits).

**Boundary (files that may be touched):** `structure-from-sherds-pp` repo only — `main` branch (reset to `11f4827`, restore, force-push), `cure-potA` branch (delete local + remote). Preprocessing repo untouched. Spartan checkouts pull-only.

**Constraints:**
- Scoring sources byte-identical to `9195c01` afterwards, except the 3 keep-hunks below.
- Keep-hunks (only allowable deltas vs pristine): (1) `class/axis_estimation.cpp` 2x Ceres `num_threads` 12->1 (determinism, Ticket-12); (2) `class/data_structure.cpp` standard-PCD fallback else-branch only (6-col parse restored; no `original_axis_norm_`, no AXIS prints); (3) `class/data_path.h` fork version kept (Pot A tables byte-identical; preserves Juglet lane).
- Delete: 30 fork-added `class/` files (incl. intersection_detector.*, ground_truth_debug.h, puzzlefusion*, GGCE*, hub, rim_base, robust_icp, multi_hypothesis, physics, pottery_structure, surface_overlap, two_phase, connectivity, enhanced_ranking, full_physics); root-added mains/validators/test_teaser; root CMakeLists.txt + main.cpp restored to pristine (E2E link-sed already handles main.cpp.o).
- Untouched: hpc/ (shim + glue), tests/, sbatches, docs, intent/, etc/, Dockerfile, Spartan-side gt_stub.cpp.
- Final `git diff 9195c01 --stat -- class/ CMakeLists.txt main.cpp` must show ONLY axis_estimation.cpp, data_structure.cpp, data_path.h.

**Acceptance criteria:** (1) `main` force-pushed, `origin/main` == restore commit, `cure-potA` gone locally and on origin; (2) fresh Spartan clone at restore HEAD builds `Hierarchy-Clear` + shim recipe (LINK-SHIM-OK, DRIVER-SHIM-OK) with a NEW sbatch (no P22FIX-grep guards); (3) same-binary ORIG control 8/8+15/15 ( Sleeping Bear: proves pristine behavior restored); fresh arm reported as diagnostic only.

**Required evidence:** restore commit hash; pre/post `git diff 9195c01 --stat`; push/branch-deletion log; Spartan job IDs + sacct states; ORIG + fresh tallies; seating render.

## Removed-commit record (copied here before the reset, per conservator Q3)

COMMIT 9f432bc0ea29e28c0a8c71ce81eeb318860585a6 — P22FIX2: CheckGraph site2 S 50->150. Symptom: site2 overlap veto at paper S=50 rejected true long-seam graphs (p8 starvation, tickets 24-26). Proof: sfs_p22fix_src ORIG-validated; ticket-27 cure 8/8+15/15 (a5d9ccd); ticket-28 byte-identical, second 50 site untouched. Decision: land 150.

COMMIT 89f771c26f1b91fe50d39934272d599d367db5c5 — P22FIX: drop || overlap_ veto in RegistrationPruning (feature_matching.cpp:1989). Symptom: RP overlap-kill vetoed true pairs before CheckGraph (p8/p3 starvation; 2-8 support died in RP). Proof: same cure lineage; diag prints + step_counter widening left out. Decision: land; CheckGraph still guards.

COMMIT b35905b421e875169130b0cc1f264e03af1b57df — C2-restore: pairwise axis gate 0.610 -> true-upstream 0.436 (feature_matching.cpp:1773). Symptom: ac64c3f "upstream 0.610" false (true upstream sfs_p22fix_src:1715 = 0.436); admitted pairs collapsed ORIG to 0/8 (ticket-28 job 33145079 ORIG 2/8+1/15; ticket-29 ORIG 0/8 job 33152327). Proof: abl2 restore job 33153903 causal ORIG 0/8->8/8+14/15 (pruned 124/63, strict 10/15). Decision: restore 0.436.

Answers: S1

Status: done (lead-verified 2026-10-20: criteria 1-3 green, see Comments; VERDICT-matcher spacing fix outstanding as follow-up)

## Comments (lead, 2026-10-20)

**Surgery LANDED (lead-executed, laptop):** reset `main` to `11f4827`; `git checkout 9195c01 -- CMakeLists.txt main.cpp class/`; `git rm` 37 compiled-in fork files (30 class/* incl. intersection_detector.*, ground_truth_debug.h, puzzlefusion*, GGCE*, hub, rim_base, robust_icp, multi_hypothesis, physics, pottery_structure, surface_overlap, two_phase, connectivity, enhanced_ranking, full_physics + root main_headless.cpp, main_headless_correct.cpp, pottery_geometric_validator*, test_teaser.cpp); re-applied 3 keeps (axis_estimation 2x Ceres 1-thread, data_structure PCD fallback, fork data_path.h); verified `git diff 9195c01 --stat -- class/ CMakeLists.txt main.cpp` = ONLY axis_estimation.cpp, data_path.h, data_structure.cpp. Committed `b56bc80` (Restore pristine assembly scorer; keep Spartan-run glue only), force-pushed `origin/main` (b35905b..b56bc80), deleted `cure-potA` locally and on origin. hpc/, tests/, sbatches, docs, intent/ untouched.

**Spartan proof (dispatched, pending):** fresh clone at `b56bc80`, NEW sbatch (no P22FIX-grep guards), shim recipe (LINK-SHIM-OK + DRIVER-SHIM-OK + snapshot gate + cwd capture), E2E k=5/b=3 both arms on the ticket-38 wall-only bundle; ORIG 8/8+15/15 required, fresh diagnostic.

**Lead verification (2026-10-20) — VERDICT: PASS.** Independently checked: origin/main == b56bc80, cure-potA ref absent; laptop HEAD == b56bc80; slurm-33502035.out shows ORIG Sherd 8/8 + Edge 15/15 and fresh 8/8+15/15 (pruned 124/70 + 420/119); run-dir logs show LINK-SHIM-OK / LINK-FORKMAIN-ABSENT-OK / DRIVER-SHIM-OK x2 arms, SNAPSHOT-T-COUNT=8; lead eye on seat_orig_vs_gt.png confirms all 8 fragments hug the GT shell. Caveat: sbatch VERDICT line prints VOID from its own spacing bug — evidence lines authoritative; matcher fix is follow-up work, not a blocker.

## Comments (worker, 2026-10-20)

**PROOF RUN — PASS (job 33502035; first attempt 33501729 failed on fork-era glue, see below).**

- Fresh clone: `/data/gpfs/projects/punim2657/pota_restore_e2e/src`, HEAD `b56bc80d788fd10110d2a821097a29bcdba759e6`, tree clean. NEW sbatch `run_pota_restore_e2e.sbatch` in the same new dir (template: pota38 abl2, cure-greps replaced by restore guards).
- Guards (host + in-container, both green): HEAD==b56bc80, tree clean; ABSENT P22FIX / P22FIX2 / POTA38 / `axis_angle_ > 0.610` / `isSimilarTrans 0.2`; PRESENT `axis_angle_ > 0.436` x2 (no POTA38 comment), PRISTINE-RESTORE keeps (2x `num_threads = 1`, PCD fallback). Bundle value-compare vs `pota28_bun_md5.txt`: OK.
- Build: `Hierarchy-Clear` 100% + shim `TOP_k=5/BRANCH_b=3` link-swap; LINK-SHIM-OK, LINK-FORKMAIN-ABSENT-OK, DRIVER-SHIM-OK, SNAPSHOT-T-COUNT=8 — both arms.
- Tallies (same job, same source): ORIG 8/8 + 15/15; fresh 8/8 + 15/15 (diagnostic). Pruned/hypothesis: ORIG 124/70, fresh 420/119.
- Strict (laptop, Kabsch-aligned, diagnostic): ORIG 6/8 sherds ≤5°/≤10mm (rot all ≤2.3°; P5 10.5mm, P7 10.1mm over); fresh 7/8 (P5 12.1mm over). Full tables in worker report.
- Renders eye-checked: `artifacts/pota_restore_e2e/seat_orig_vs_gt.png`, `seat_fresh_vs_gt.png` — all 8 fragments hug the GT shell, coherent vessel, nothing scattered.
- RUNDIR: `/data/gpfs/projects/punim2657/pota_restore_e2e/pota_restore_run_33502035/` (+ fetch tarball `pota_restore_fetch.tgz`). sacct: 33502035 COMPLETED 0:0.
- Notes: (1) job 33501729 (COMPLETED 0:0 outer) failed both arms compiling fork-era `pota29/gt_stub.cpp` — the restore deleted `class/ground_truth_debug.h`, and the clean `Hierarchy-Clear` link proves nothing references `GroundTruthDebugger`; sbatch now skips gt_stub.o when the header is absent (prints GTSTUB-SKIPPED-OK). Still committed-sources-only. (2) The sbatch VERDICT line says VOID due to a spacing bug in its own match (`8/8` vs printed `8 / 8`); the evidence lines above it read 8/8+15/15 — verdict here is PASS. Verdict: **PASS — restore behaves pristine; ORIG 8/8+15/15 in the same job.**

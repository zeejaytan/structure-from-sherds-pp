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

## Status: results-in (lead-executed 2026-10-20, see Comments)

## Comments (lead, 2026-10-20)
Placeholder — surgery + proof evidence appended here.

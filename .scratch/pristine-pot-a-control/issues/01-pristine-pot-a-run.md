# 01: Pristine Pot A run (control arm)

**What to build:** From a conservator's view, one clean end-to-end check that the authors' own software, untouched, can join the one ordinary round pot (Pot A) it was built for — so the Juglet zero can be read as material scope, not a broken instrument. The worker starts from the pristine archive tree, binds the original Pot A sherds, first replays the truth-placement join-gate probe, then runs one full assembly, stages one joined pair for the eye, and writes the numbers back to S1. No tuning, no new code paths.

**Answers:** S1

**Blocked by:** None (can start immediately — after the 4 preconditions below are confirmed by the lead)

**Status:** ready-for-agent

**Spec:** `../spec.md` (pristine Pot A control spec; this ticket executes its capability-half arm only — no code edits, no intent edits by the worker beyond the S1 write-back line)

**SHA authority note:** spec line 15 cites `FORK_VS_UPSTREAM.md:15` as pinning `upstream/main` SHA `9195c01`. If that citation does not match the assembly-side audit, the worker records the **archive-clone measured SHA** (`git -C <archive_2025_sfs_modified_src> rev-parse HEAD` + `git status --porcelain`) as authoritative and notes the discrepancy in the ticket Comments — the measured clone wins over the citation.

## Preconditions (spec Open items — lead confirmed 2026-10-14, evidence from teammates image-entry-probe + bind-workspace-probe)

- [x] O1 — `.sif` build recipe confirmed: NO pristine-capable recipe exists in-tree. `hpc/containers/` holds exactly 3 recipes: `container.def` (preprocessing only, `:5-6,43-45,60-71`), `container-base.def` (deps only, no source), `container-preprocessing.def` (closest shape, but bakes fork source — `%files :17-19` copy `sfspreproc-docker/` working copy, needs source-swap + audit). Every assembly sbatch uses `/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif` with undocumented provenance (`hpc/README.md:13-14`: images stay unversioned). Prescribed path: worker writes a minimal recipe cloning upstream @ `9195c01` (include X libs for O4 Rank 1) — NOT reuse of `sfspreproc.sif`, NOT `build_assembly.sh` (fork rebuild helper, `:12,17,42-46`).
- [x] O2 — `/Dataset` bind confirmed: host `/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp` → container `/Dataset/SfS_pp` (satisfies `class/data_path.h:1614` `POTA_ORIG_BASE`). Verbatim flag: `--bind "/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp:/Dataset/SfS_pp"` (pattern proven by repo-root `run_legacy_sfs.sbatch:57-58`, the only literal bind of its kind; `hpc/sbatch/*` binds point at `sfs_preprocessing/...` or `${FRESH}`/`${DATASET_PATH}` — `run_e2e_pota_fresh.sbatch:45-46`, `rebuild_top_k.sbatch:70-71`; `run_pot_a_original.sbatch:41-42` is non-conforming, binds to same host path instead of `/Dataset/SfS_pp`).
- [x] O3 — independent workspace confirmed with caveat: Spartan-side `/data/gpfs/projects/punim2657/archive_2025_sfs_modified_src` (per spec:15; never `sfs_legacy_temp` — `AGENTS.md:20` "Leave alone"). CAVEAT: live Spartan `ls`/SHA check was unavailable from these sessions (ssh_list malformed, ssh_exec "Invalid username") — worker MUST verify on-cluster (`ls` archive dir + `git rev-parse HEAD` + `status --porcelain`) before running, and confirm whether it is the relocated `sfs_main/sfs_modified_src` tree (`AGENTS.md:21`, `hpc/README.md:33-34`) or a sibling. Laptop side clean: `main @ 497b9d2`, `diff --stat` empty, only these two ticket files untracked — no over-edit.
- [x] O4 — headless entry confirmed: Rank 1 = pristine + virtual display (Xvfb/VNC/X11), zero source change (upstream `main.cpp:43` viewer + `:253-254` viewer-run-loop verified); precedent `hpc/scripts/setup_vnc_access.sh:29` `export DISPLAY=:1`. Fallback Rank 2 = fresh minimal shim against the pristine tree (`main_batch.cpp` pattern `:16,42-43,133,160,176-186`, never verbatim — it is fork-era and `CMakeLists_batch.txt:4-12` makes Ceres optional, a solver-behavior risk), declared INFRA with diff. EXCLUDED: fork `main_headless*.cpp` (`FORK_VS_UPSTREAM.md:55-62,237,260-261`; `CMakeLists.txt:166-168`) — carries fork behavior deltas. Worker verifies a display works on the compute node before committing to Rank 1.

## Acceptance

- [ ] Isolation: run executes in the O3 workspace only; evidence pasted: `git -C <pristine-tree> rev-parse HEAD` (= `9195c0167e2077a564bd7c91156063252feeb506`, short `9195c01`, or measured value per SHA note), `git status --porcelain` empty output, workspace path.
- [ ] Binding: input is `sfs_main/original_samples/SfS_pp` `Pot_A_*` = 8 sherds / 15 edges per `class/data_path.h:1610-1707` (`SHARD_NUMBER 8`, GT 8 pieces + `Pot_A_simple_graph.txt`); evidence: file count + Breaklines/Mesh/Axes/Surfaces×2/Surface_F/GT listing, sbatch `--bind` line.
- [ ] Purity: zero `class/` algorithm/threshold/weight/solver changes vs the pinned SHA (`git diff --stat` empty except declared INFRA shims); every shim listed with path + one-line INFRA reason (dataset-select macro, `/Dataset` bind, build script, O4 entry); no preprocessing产物混入.
- [ ] Gate probe reported: ground-truth-placement join-gate pass rate on Pot A (bar: 15/15 per S1 precedent vs Juglet 0/18); command + log excerpt pasted (placeholder: `python <probe>.py /Dataset/SfS_pp --pot POT_A_ORIG 2>&1 | tee gate_probe.log` — worker replaces `<probe>.py` with lead-confirmed probe path per spec seam S2, fork-side read-only if so declared).
- [ ] E2E reported at Table V Pot A gauge: single full assembly (`k=5,b=3`) scored SA/EA against paper Table V `A (8)` SfS++ SA 100 / EA 100 (`papers/text/sfspp-2502.13986v1.md:344-356`, V-B `τR=20° τt=50mm :374-378`); report SA x/8, EA x/15, best score, job ID + `sacct` State/ExitCode, result dir. A 0-result honestly reported counts as a valid answer. **87.3% is NOT the bar** (it is the 10-pot `ALL (142)` mix, Table VII `:400-407,411`) — ticket states this explicitly in its report.
- [ ] Needs-eye: one joined pair (or best miss if 0 joins) staged as a visual-qa pair and witnessed before numbers are trusted.

**Needs-eye:** `visual-qa/preview/manifest*.json` pair staged by worker (one join render, view resolving the seam scale) + conservator note in the pair log; image-only staging stays out of the `manifest*.json` registry per `docs/agents/issue-tracker.md`.

- [ ] S1 write-back: `intent/S1-does-it-connect-anything-real.md` updated with date, gate rate, E2E SA/EA, job ID, and capability-half sentence (proves pipeline works on suitable material; claims nothing universal); `python ../scripts/check_intent_links.py` gate 0 errors (placeholder: `python ../scripts/check_intent_links.py` run from `structure-from-sherds-pp/`, output pasted); board rebuild per AGENTS.md.

## Gate commands (placeholders — worker fills paths/IDs)

```bash
python ../scripts/check_intent_links.py
git -C <archive_2025_sfs_modified_src> rev-parse HEAD && git -C <archive_2025_sfs_modified_src> status --porcelain
python <probe>.py /Dataset/SfS_pp --pot POT_A_ORIG 2>&1 | tee gate_probe.log
```

## Comments

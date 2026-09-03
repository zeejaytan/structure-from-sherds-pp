# AGENTS.md — structure-from-sherds-pp (project)

Follow the workspace root **`../AGENTS.md`** (laptop ↔ GitHub ↔ Spartan) for all shared rules. This file only adds project-specific paths and domain notes.

## Paths

| Role | Value |
|------|--------|
| GitHub fork (`origin`) | `zeejaytan/structure-from-sherds-pp` |
| Upstream | `SeongJong-Yoo/structure-from-sherds-pp` |
| Spartan checkout (`REMOTE_ROOT`) | `/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker` |
| SSH | `Host spartan`, user `zhuojiat` |
| Remote helpers | `scripts/remote/pull_and_sbatch.sh`, `job_status.sh`, `fetch_artifacts.sh` |

**Naming warning:** despite the folder name, `sfspreproc-docker` is the **assembly** code (SFS++ / `Hierarchy-Clear`), *not* preprocessing. Actual preprocessing lives in the separate `sfs_preprocessing` repo (`zeejaytan/SfSpp_preprocessing`).

## Surrounding Spartan layout

- `sfs_main/` (parent dir) — the launch/analysis working area. Its scripts, docs, tools, and patches are **versioned in this repo under `hpc/`** (see `hpc/README.md`); the live copies on Spartan stay untracked there and are launched from `sfs_main/` (their paths reference `sfspreproc-docker/` as a subdirectory). Edit in `hpc/`, push, sync the live copy. Results/logs/datasets in `sfs_main/` remain HPC-only.
- `sfs_main/sfs_legacy_temp/` — second clone of this fork used for tray-000 threshold debugging. Leave alone.
- `sfs_main/sfs_modified_src/` — pristine clone of upstream. Reference only.

## Repo conventions / gotchas

- `main_headless_correct_branch_{N}.cpp` are **generated** — `run_top_k_parallel.sbatch` (outer `sfs_main/`) copies `main_headless_correct.cpp` per Slurm array task and appends `PARALLEL_BRANCH_ID`. Gitignored; never commit or edit them — edit `main_headless_correct.cpp`.
- `CMakeLists_enhanced.txt` was an older draft fully merged into the tracked `CMakeLists.txt` (verified zero unique lines); it was deleted during the 2026-07-19 migration.
- `surface_overlap_detection.{cpp,h}` (repo root) is an early standalone draft, superseded by the tracked `class/surface_overlap_detector.{cpp,h}` which is what the build uses. Kept for reference only; nothing includes it.
- GGCE (Global Graph Connectivity Enhancement) is documented in `GGCE_README.md`; its tests live in `tests/` and build as `ggce_tests` via the tracked `CMakeLists.txt`.
- Historical analysis/debugging writeups (`ROOT_CAUSE_DIAGNOSIS.md`, `COORDINATE_FIX_PROGRESS.md`, etc.) live at repo root from the research phase.

Typical loop:

```bash
git push origin HEAD
./scripts/remote/pull_and_sbatch.sh run_legacy_tray000.sbatch
./scripts/remote/job_status.sh
./scripts/remote/fetch_artifacts.sh logs ./artifacts/
```

## Agent skills

Configured here so this repo works when opened on its own, not only from the `C:\PR`
umbrella. The full text of each convention lives at the workspace root; these are the
parts an agent needs before it can act.

- **Issue tracker — local markdown.** One feature per directory: the spec at
  `.scratch/<feature>/spec.md`, tickets one per file at
  `.scratch/<feature>/issues/<NN>-<slug>.md`, numbered from `01` in dependency order.
  Every ticket carries an **`Answers:`** line naming the question in `intent/` it exists
  to settle -- `S1` for this project, `U6` for the workspace, or `none` for routine
  work. Conventions and the ticket template: `../docs/agents/issue-tracker.md`.
- **Triage labels.** `needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`,
  `wontfix`, recorded as a `Status:` line near the top of the ticket. Details:
  `../docs/agents/triage-labels.md`.
- **Domain docs — single-context.** Three different things, kept apart: **this file** is
  how to work here and the traps; **`CONTEXT.md`** at the repo root is the glossary, and
  `/domain-modeling` creates it lazily when the first term is actually resolved — do not
  create it empty; **`../docs/glossary.md`** is the cross-project measurement vocabulary
  (`part_acc`, chamfer distance, best-of-N) and outranks any local redefinition. ADRs go
  under `docs/adr/`. Details: `../docs/agents/domain.md`.
- **Intent.** [`intent/`](intent/) holds what we are trying to establish and what would
  settle it -- prefix **`S`**, permanent, numbers never reused. `/to-intent` opens a
  question or writes a finished ticket's result back into one. Check the loop is wired
  with `python ../scripts/check_intent_links.py`.

**Do not run `/setup-matt-pocock-skills` in this repo.** It would replace the above with
its own defaults, and its ticket template has no `Answers:` line -- tickets would stop
being connected to the question they exist to answer, silently.

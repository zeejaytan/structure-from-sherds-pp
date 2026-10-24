# 03: Move research-phase notes and root batch scripts out of the root; untrack .DS_Store

**What to build:** The repo root holds only what builds and describes the repo: the 16 research-phase notes move to `docs/notes/`, the 12 root batch scripts to `hpc/sbatch/`, and `.DS_Store` is untracked and ignored.

**Answers:** none (workbench routine).

**Blocked by:** 02.

**Status:** resolved

Amendments 2.2 and 2.4. Check every moved path for references (`git grep`) and fix them in the same commit.

- [x] Root lists only build files, `README`, `AGENTS.md`, `CLAUDE.md`, source dirs
- [x] `git grep` finds no stale path
- [x] `.DS_Store` untracked and in `.gitignore`

## Result (2026-10-24)

The counts in the brief were estimates; the actual moves, all with `git mv`:

- 13 research-phase notes → `docs/notes/` (the brief said 16; `README.md`, `AGENTS.md`,
  `CLAUDE.md` and `LICENSE` stay);
- 12 root batch scripts → `hpc/sbatch/` (no name collisions);
- 6 root Python scripts → `hpc/analysis/`, and 9 run/harvest shell scripts → `hpc/scripts/`,
  which the brief did not list but which are not build files either.

Kept at the root as build files: `CMakeLists.txt`, `main.cpp`, `Dockerfile`,
`ceres-solver-1.14.0.tar.gz`, `build_assembly.sh`, `build_at_container.sh`,
`build_baseline.sh`, `build_hierarchy.sh`, `setup_container.sh`, `download.sh`.

None of the moved scripts finds files relative to its own location: every batch script `cd`s
to an absolute Spartan path, and the shell scripts use absolute roots or none. References
fixed: `AGENTS.md` (GGCE note, research-notes line, typical loop), `intent/README.md` and
`intent/S1` (10 citations now `docs/notes/...`), one `print` in
`hpc/analysis/analyze_tray000_results.py`, and the inventory row in
`docs/notes/FORK_VS_UPSTREAM.md`. Notes that cite each other by bare name still resolve,
being siblings. Notes that `cd sfs_main/sfs_legacy_temp` and run a root script describe that
older clone, where the files are still at its root; left as they are, and `AGENTS.md` says
so. Closed tickets under `.scratch/` keep the paths they had when written. Layout gate
(`check_workspace_layout.py`): 7 checks, 6 passed, 1 skipped.

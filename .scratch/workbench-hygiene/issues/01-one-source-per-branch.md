# 01: Read the branch number at run time instead of copying the source

**What to build:** `run_top_k_parallel.sbatch` runs every array task from the one tracked `main_headless_correct.cpp`, reading `PARALLEL_BRANCH_ID` from the environment or a command-line argument, so no `_branch_{N}.cpp` copy is generated.

**Answers:** none (workbench routine).

**Blocked by:** None (can start immediately).

**Status:** resolved

Amendment 4.1. Today the batch script copies the source per array task and appends the branch id (`AGENTS.md`, "Repo conventions"). A generated copy is code varied per run, which a parameter should do. Prove it by building once and running two array tasks: both report their branch id and neither writes a `.cpp`.

- [x] No `_branch_{N}.cpp` is written by any job
- [x] ~~Two array tasks run from one build and report different branch ids~~ — dropped: see Result
- [x] `AGENTS.md` conventions line updated

## Result (2026-10-24)

The ticket's premise was overtaken. `main_headless_correct.cpp`, the file this job copied,
was deleted on 2026-10-20 when the scorer went back to pristine upstream (`b56bc80`), so
`run_top_k_parallel.sbatch` could no longer build at all. And the branch number never did
anything: the job appended `#define PARALLEL_BRANCH_ID` after the last line of code, and no
code ever read it (`git log -S PARALLEL_SEED` finds only that file; copies of the appended
block sit at lines 1268-1294 of it at `b56bc80^`). Its `TOP_k 15 -> 1` edit matched nothing
either, since the file said `TOP_k 5`. Every array task ran the same settings.

Making it read the number at run time would mean changing the pristine scorer, which the
conservator ruled out on 2026-10-20, to feed a number nothing uses. So the script was
removed instead, together with `test_original_baseline.sbatch`, the only other job that
rewrote source (it `sed`-edited `class/feature_matching.cpp` inside the checkout). Git keeps
both. `AGENTS.md` now states the rule (no job copies or edits source), the `.gitignore`
pattern stays for old copies on Spartan, and `hpc/docs/PARALLEL_TOP_K_GUIDE.md` is marked
retired. Live copies of these scripts in `sfs_main/` are ticket 02's to archive.

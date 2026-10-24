# 01: Read the branch number at run time instead of copying the source

**What to build:** `run_top_k_parallel.sbatch` runs every array task from the one tracked `main_headless_correct.cpp`, reading `PARALLEL_BRANCH_ID` from the environment or a command-line argument, so no `_branch_{N}.cpp` copy is generated.

**Answers:** none (workbench routine).

**Blocked by:** None (can start immediately).

**Status:** ready-for-agent

Amendment 4.1. Today the batch script copies the source per array task and appends the branch id (`AGENTS.md`, "Repo conventions"). A generated copy is code varied per run, which a parameter should do. Prove it by building once and running two array tasks: both report their branch id and neither writes a `.cpp`.

- [ ] No `_branch_{N}.cpp` is written by any job
- [ ] Two array tasks run from one build and report different branch ids
- [ ] `AGENTS.md` conventions line updated

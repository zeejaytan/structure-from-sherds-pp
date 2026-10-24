# 02: Submit from the repo's own batch scripts; retire the hand-synced live copies

**What to build:** Jobs are submitted from `hpc/` in the canonical checkout (or through `new_run.sh`), with absolute log paths into the run directory. The untracked live copies in `sfs_main/` are archived to the attic and removed.

**Answers:** none (workbench routine).

**Blocked by:** 01 (touches the same batch scripts).

**Status:** ready-for-agent

Amendments 4.2 and 2.3. Two copies of a batch script drift, and the run's `submit.sbatch` is already the record of what was submitted. Before removing the live copies: `diff` each against its `hpc/` twin and commit any difference that is real; then archive them under rule 13.

- [ ] Every live copy diffed; real differences committed to `hpc/`
- [ ] Live copies archived (proved) and removed
- [ ] No batch script uses a relative `--output`

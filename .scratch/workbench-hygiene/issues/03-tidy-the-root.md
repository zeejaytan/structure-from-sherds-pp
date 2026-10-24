# 03: Move research-phase notes and root batch scripts out of the root; untrack .DS_Store

**What to build:** The repo root holds only what builds and describes the repo: the 16 research-phase notes move to `docs/notes/`, the 12 root batch scripts to `hpc/sbatch/`, and `.DS_Store` is untracked and ignored.

**Answers:** none (workbench routine).

**Blocked by:** 02.

**Status:** ready-for-agent

Amendments 2.2 and 2.4. Check every moved path for references (`git grep`) and fix them in the same commit.

- [ ] Root lists only build files, `README`, `AGENTS.md`, `CLAUDE.md`, source dirs
- [ ] `git grep` finds no stale path
- [ ] `.DS_Store` untracked and in `.gitignore`

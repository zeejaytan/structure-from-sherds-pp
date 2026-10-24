# Workbench hygiene in SfS++ assembly

**Prior work searched:** GitHub and git's own manual for worktree practice (https://git-scm.com/docs/git-worktree, 2026-10-24); CMake's guidance on out-of-source builds; the Spartan data-management page (https://dashboard.hpc.unimelb.edu.au/data_management/). Found: no released code applies; this is our own workbench, following `docs/agents/workspace-hygiene.md` in pr-workspace (`../docs/agents/` from this repo on the laptop).

**Answers:** none. This effort carries out this repo's share of the workbench amendments the conservator agreed on 2026-10-24 ("Workbench amendments against the five habits", https://claude.ai/artifact/URhGpYZcyeXf2LvfNwtz8y). Tickets that touch the same files run in order, in one thread.

## Tickets

- `issues/01-one-source-per-branch.md` — Read the branch number at run time instead of copying the source.
- `issues/02-submit-from-the-repo.md` — Submit from the repo's own batch scripts; retire the hand-synced live copies.
- `issues/03-tidy-the-root.md` — Move research-phase notes and root batch scripts out of the root; untrack .DS_Store.

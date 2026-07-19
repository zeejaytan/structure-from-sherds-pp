# CLAUDE.md — structure-from-sherds-pp (project)

Follow the workspace root **`../AGENTS.md`** / **`../CLAUDE.md`** (laptop ↔ GitHub ↔ Spartan) for all shared rules. Same overlay as **`AGENTS.md`** in this folder — the paths table, the misleading-name warning (this is the **assembly** code, not preprocessing), and repo gotchas (generated `main_headless_correct_branch_*.cpp`, superseded drafts) live there.

Edit and commit on the laptop; Spartan is pull-only (`git pull --ff-only`) and runs Slurm via `scripts/remote/*`. Heavy data and results stay on Spartan; `artifacts/` is the local, gitignored rsync landing zone.

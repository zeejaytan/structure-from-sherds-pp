# 02: Submit from the repo's own batch scripts; retire the hand-synced live copies

**What to build:** Jobs are submitted from `hpc/` in the canonical checkout (or through `new_run.sh`), with absolute log paths into the run directory. The untracked live copies in `sfs_main/` are archived to the attic and removed.

**Answers:** none (workbench routine).

**Blocked by:** 01 (touches the same batch scripts).

**Status:** resolved

Amendments 4.2 and 2.3. Two copies of a batch script drift, and the run's `submit.sbatch` is already the record of what was submitted. Before removing the live copies: `diff` each against its `hpc/` twin and commit any difference that is real; then archive them under rule 13.

- [x] Every live copy diffed; real differences committed to `hpc/`
- [x] Live copies archived (proved) and removed
- [x] No batch script uses a relative `--output`

## Progress (2026-10-24)

Spartan inventory (read-only, relayed): `sfs_main/` holds 125 top-level `*.sbatch`, `*.sh`
and `*.py`. Against `hpc/{sbatch,scripts,analysis}` at `origin/main` 40b3529: **119
byte-identical, 0 differing, 6 with no twin**. The six (`run_pristine_juglet.sbatch`,
`onset_table.py`, `profile_battery.py`, `which_edge_passed.py`, `which_edge_row1.py`,
`which_pieces_right.py`) were copied out with `cp -p`, md5-checked against Spartan's sums,
and committed to `hpc/sbatch/` and `hpc/analysis/`. No running job has its WorkDir under
`sfs_main/`; newest script mtime 2026-10-15 01:40. Files: `/mnt/project-files/sfs_hygiene_*`.

83 relative `#SBATCH --output`/`--error` lines in 44 scripts now carry
`/data/gpfs/projects/punim2657/sfs_main/` in front, which is where they landed when
submitted from there, so submitting from the checkout changes nothing about where logs go.
`new_run.sh` passes its own `--output` on the command line, which wins over the header.

Found on the way, not this ticket's to fix: the canonical checkout `sfs_main/sfspreproc-docker`
sits at `11f4827` (an ancestor of main) with two hand edits from 2026-10-16 (P22FIX in
`class/feature_matching.cpp:1989`, P22FIX2 in `class/ranking_system.cpp:1838`), which the
2026-10-20 pristine decision superseded. They block `git pull --ff-only`. Put to the
conservator with the archive step.

Done 2026-10-24 on the conservator's "Clear both" (decision card, 08:50Z), by the Spartan
workspace thread following `/mnt/project-files/workbench-hygiene/sfs02_spartan_steps.md`;
log `/mnt/project-files/sfs02_archive_log.txt`. Tarball 125 members, `gzip -t` ok, `cmp`
ok on all 125; guards clear (no job WorkDir under `sfs_main/`, nothing modified in the last
hour); 125 removed, 0 left. The two hand edits saved as
`SfSpp/attic/sfspreproc-docker-P22FIX-handedits-20261024.patch` (26 lines, `apply -R --check`
ok), reverted, and the checkout pulled `11f4827` → `40b3529`. Attic:
`/data/gpfs/projects/punim2657/SfSpp/attic/sfs_main-live-scripts-20261024{.tar.gz,-INDEX.txt,-deleted-manifest-20261024.tsv.gz}`;
row `sfs_main-live-scripts-20261024` in `SfSpp/runs/index.tsv`. Untracked test binaries and
`build_sif/` in the checkout left as they were.

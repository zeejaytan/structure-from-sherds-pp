# hpc/ — versioned copy of the Spartan `sfs_main/` working area

This tree is the **versioned source** for the research working area that lives at
`/data/gpfs/projects/punim2657/sfs_main/` on Spartan (the parent directory of this
repo's cluster checkout, `sfs_main/sfspreproc-docker/`). It was folded into the repo
during the 2026-07 workspace migration so the scripts and writeups share history with
the assembly code they operate on.

**How jobs are submitted (changed 2026-10-24, workbench-hygiene 02).** Jobs run from the
scripts here, in the canonical checkout: `sbatch hpc/sbatch/<job>.sbatch` from
`sfs_main/sfspreproc-docker/`, or through the workspace's `scripts/new_run.sh`, which
records the run. Each script `cd`s to `/data/gpfs/projects/punim2657/sfs_main` itself and
its `#SBATCH --output`/`--error` paths are absolute, so the submit directory does not
matter. The live copies at the top of `sfs_main/` are retired: of 125, 119 were byte-identical to
these and the other 6 have now been added. On 2026-10-24 all 125 were archived to
`/data/gpfs/projects/punim2657/SfSpp/attic/sfs_main-live-scripts-20261024.tar.gz` and removed. Two copies of
a batch script drift; a run's `submit.sbatch` is the record of what was submitted.

Results, logs, datasets, containers (`.sif`), and build dirs stay unversioned on the
cluster.

## Layout

| Dir | Contents |
|-----|----------|
| `docs/` | Research writeups: breakthrough reports, PRDs (`*PRD*.md`), root-cause analyses, tuning/cleanup logs, idea notes. `SFS_MAIN_LAYOUT.md` (formerly `sfs_main/CLAUDE.md`) explains the cluster directory layout and the misleading `sfspreproc-docker` name. |
| `sbatch/` | Slurm job scripts: SFS runs, top-K parallel exploration, phase tests, NURBS pipelines, pottery/physics validation. Submit from the canonical checkout (`sbatch hpc/sbatch/<job>.sbatch`); each script `cd`s to `sfs_main/` itself. |
| `scripts/` | Shell helpers: builds (`build_*`), result aggregation (`aggregate_*`), preprocessing generation/fix scripts, monitors. |
| `analysis/` | Python analysis of run results: adjacency/connectivity, physics, scoring, ground-truth comparison, visualization generators. |
| `tools/` | Standalone C++ utilities with their `CMakeLists_{axis,batch,tps}.txt`: edgeline extraction, TPS/NURBS surface generation, batch main, small `test_*.cpp` probes. `edgeline_extraction.cpp` and `generate_nurbs_compatible_surface_f.cpp` are **variants** of same-named files in the `SfSpp_preprocessing` repo — diverged copies, kept as-is. |
| `patches/` | Fix drafts against the assembly code (`implementation_patch.*`, `optimizeGlobalTransformations_FIXED.cpp`, `top_k_transformation_solution.cpp`, `pottery_integration.cpp`, `debug_lcs_failure.patch`). These may be partially or fully merged into `class/` already — check before applying. |
| `containers/` | Apptainer/Singularity `.def` recipes (`container*.def`). Built `.sif` images stay on the cluster. |

## What was not brought over

From `sfs_main/`: `results*`, `visual_output*`, `parallel_results_branch_*`,
`original_samples*`, backups/archives, job logs (`*.out`/`*.err`/`*.log`),
`sfspreproc.sif`, `cache/`, `Dataset/`, `Temp/`, build directories, and the three
nested repos (`sfspreproc-docker/` = this repo, `sfs_legacy_temp/`,
`sfs_modified_src/`). `pottery_geometric_validator.{cpp,h}` were skipped as
byte-identical to the repo-root copies.

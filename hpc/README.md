# hpc/ — versioned copy of the Spartan `sfs_main/` working area

This tree is the **versioned source** for the research working area that lives at
`/data/gpfs/projects/punim2657/sfs_main/` on Spartan (the parent directory of this
repo's cluster checkout, `sfs_main/sfspreproc-docker/`). It was folded into the repo
during the 2026-07 workspace migration so the scripts and writeups share history with
the assembly code they operate on.

**Operational convention (same as the TORA repo):** the *live* copies on Spartan stay
untracked in `sfs_main/` and are launched from there — their relative paths assume the
repo is a subdirectory (e.g. `sbatch` scripts reference `sfspreproc-docker/...`).
When changing a script: edit it here, push, then update the `sfs_main/` copy to match.
Results, logs, datasets, containers (`.sif`), and build dirs were deliberately left
unversioned on the cluster.

## Layout

| Dir | Contents |
|-----|----------|
| `docs/` | Research writeups: breakthrough reports, PRDs (`*PRD*.md`), root-cause analyses, tuning/cleanup logs, idea notes. `SFS_MAIN_LAYOUT.md` (formerly `sfs_main/CLAUDE.md`) explains the cluster directory layout and the misleading `sfspreproc-docker` name. |
| `sbatch/` | Slurm job scripts: SFS runs, top-K parallel exploration, phase tests, NURBS pipelines, pottery/physics validation. Launch from `sfs_main/` on the cluster, not from inside the repo. |
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

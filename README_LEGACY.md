# SFS++ Legacy System - Headless Execution

This directory contains the **original author's SFS++ code** from GitHub with modifications to run headless on the HPC cluster.

## Quick Start

```bash
cd /data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp
sbatch run_legacy_sfs.sbatch
```

## What's Inside

### Repository
- **Source**: https://github.com/SeongJong-Yoo/structure-from-sherds-pp
- **Cloned to**: `structure-from-sherds-pp/`

### Files Created
1. **main_headless.cpp** - Headless version of the original main.cpp
   - Removes PCL visualization dependencies
   - Auto-saves results without keyboard interaction
   - Outputs same results as original code

2. **CMakeLists.txt** (modified) - Builds both versions
   - `Hierarchy-Clear` - Original with visualization
   - `Hierarchy-Clear-headless` - Headless for HPC

3. **run_legacy_sfs.sbatch** - Slurm submission script
   - Builds code inside container
   - Runs on author's sample data
   - Auto-saves all results

## Dataset

**Author's Original Sample Data**:
```
/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/
├── Surfaces/           # XYZ point clouds (Pot_A_Piece_01-08)
├── Breaklines/         # PCD edge features
├── Axes/               # Vessel axis data
├── Mesh/               # OBJ mesh files
├── Ground Truth/       # Reference transformation matrices
└── Transformation/     # Ground truth transformations
```

## Parameters (Original Legacy)

From `main_headless.cpp` and `data_path.h`:
- **TOP_k = 5** - Beam search top-k parameter
- **BRANCH_b = 3** - Branching factor
- **POT_A** - 8 pottery pieces
- **NO_BASE_INFO** - Base info disabled
- **NO_RIM_INFO** - Commented out (rim info enabled)

## Output Location

Results are saved to:
```
/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Result/
```

### Files Generated
- `T_<piece>_to_<piece>.txt` - Transformation matrices
- `<num>. edge.xyz` - Assembled edge lines
- `<num>. surface.xyz` - Assembled surface point clouds
- `1. Acc.txt` - Accuracy report (vs ground truth)

## How to Visualize Results

### Option 1: MeshLab (Recommended)
```bash
# Convert XYZ to PLY for visualization
cd /data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Result/

# View individual pieces
meshlab 1.\ surface.xyz
meshlab 2.\ surface.xyz
# etc...
```

### Option 2: Python with Open3D
```python
import open3d as o3d
import numpy as np

# Load surface point cloud
data = np.loadtxt('/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Result/1. surface.xyz')
pcd = o3d.geometry.PointCloud()
pcd.points = o3d.utility.Vector3dVector(data)

# Visualize
o3d.visualization.draw_geometries([pcd])
```

### Option 3: Combine All Pieces
```python
import open3d as o3d
import numpy as np
import glob

result_dir = '/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Result/'
all_clouds = []

for i in range(1, 9):  # 8 pieces
    file = f"{result_dir}{i}. surface.xyz"
    try:
        data = np.loadtxt(file)
        pcd = o3d.geometry.PointCloud()
        pcd.points = o3d.utility.Vector3dVector(data)

        # Assign different colors to each piece
        color = np.random.rand(3)
        pcd.colors = o3d.utility.Vector3dVector([color] * len(data))

        all_clouds.append(pcd)
    except:
        print(f"Piece {i} not found")

# Visualize all pieces together
o3d.visualization.draw_geometries(all_clouds)
```

## Differences from Your Modified System

### Legacy System (This Directory)
- **Original author's algorithm** (TOP_k=5, BRANCH_b=3)
- **No pottery validation** - Uses original normal thresholds
- **No auto-agglomerative assembly** - Original incremental graph building
- **No multi-hypothesis optimization** - Single hypothesis per pair
- **Simple beam search** - Classic incremental assembly

### Your Modified System (`/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/`)
- Enhanced with pottery-aware validation
- PuzzleFusion++ inspired global optimization
- Multi-hypothesis framework
- Circular topology bonuses
- Multi-scale coherence validation

## Monitoring Jobs

```bash
# Check job status
squeue -u $USER

# View live output
tail -f sfs_legacy_<JOBID>.out

# Check errors
tail -f sfs_legacy_<JOBID>.err
```

## Expected Runtime
- **Build time**: ~5-10 minutes
- **Execution time**: ~15-30 minutes for 8 pieces
- **Total**: ~20-40 minutes

## Troubleshooting

### Build Fails
```bash
# Check build log
cat sfs_legacy_<JOBID>.err

# Common issues:
# - Missing dependencies (should be in container)
# - CMake version mismatch
# - Source file syntax errors
```

### No Results Generated
```bash
# Check if Result directory exists
ls -la /data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Result/

# Check execution log
grep "ERROR" sfs_legacy_<JOBID>.out
```

### Container Not Found
```bash
# Verify container exists
ls -lh /data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif
```

## Comparing Legacy vs Modified

To compare the original algorithm with your modifications:

```bash
# Run legacy version
cd /data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp
sbatch run_legacy_sfs.sbatch

# Run modified version (from main directory)
cd /data/gpfs/projects/punim2657/sfs_main
sbatch run_nurbs_sfs_timestamped.sbatch

# Compare results
diff /data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Result/1.\ Acc.txt \
     /data/gpfs/projects/punim2657/sfs_main/results_*/Result/1.\ Acc.txt
```

## Notes

- This is a **clean clone** of the author's original code
- No modifications to assembly algorithm (only visualization removed)
- Results should match the paper's reported performance
- Useful as a baseline for comparing your enhancements
- Your existing code in `/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/` is **unchanged**

## Citation

If using this legacy code, please cite the original paper:

```bibtex
@inproceedings{YooandLiu2024SfS,
    author    = {Yoo, Seong Jong and Liu, Sisung and Arshad, Muhammad Zeeshan and
                 Kim, Jinhyeok and Kim, Young Min and Aloimonos, Yiannis and
                 Fermüller, Cornelia and Joo, Kyungdon and Kim, Jinwook and Hong, Je Hyeong},
    title     = {Structure-From-Sherds++: Robust Incremental 3D Reassembly of
                 Axially Symmetric Pots from Unordered and Mixed Fragment Collections},
    journal   = {arXiv},
    year      = {2025},
}
```

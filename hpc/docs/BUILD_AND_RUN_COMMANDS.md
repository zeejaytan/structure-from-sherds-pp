# SFS Build and Execution Commands

## Building the Fixed SFS Binary

### 1. Copy Fixed Source Files
```bash
cd /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
cp ../sfs_modified_src/class/ranking_system.cpp class/
cp ../sfs_modified_src/main_headless_correct.cpp .
```

### 2. Fix CMakeLists.txt (avoid multiple main definitions)
```bash
rm -f main.cpp
cp CMakeLists.txt CMakeLists.txt.backup
sed 's/main\.cpp/main_headless_correct.cpp/g' CMakeLists.txt.backup > CMakeLists.txt
```

### 3. Build in Container
```bash
CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"
$CONTAINER_PATH exec --bind /data/gpfs/projects/punim2657/sfs_main:/workspace /data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif /bin/bash -c "cd /workspace/sfspreproc-docker && rm -f Hierarchy-Clear && make clean && cmake . && make -j4"
```

## Running the Fixed Pipeline

### Complete TPS Preprocessing Generation
```bash
cd /data/gpfs/projects/punim2657/sfs_main
./generate_complete_tps_preprocessing.sh
```

### Run SFS Assembly - SLURM Method (Recommended)

**Using Clean TPS Dataset (Recommended)**:
```bash
# Create SLURM job to use clean TPS dataset directly
sbatch test_clean_tps_relaxed.sbatch
```

**Manual Container Method**:
```bash
CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"
timeout 180 $CONTAINER_PATH exec \
  --env VTK_DEBUG_LEAKS=1 \
  --env VTK_SILENCE_GET_VOID_POINTER_WARNINGS=0 \
  --env LIBGL_ALWAYS_SOFTWARE=1 \
  --bind /data/gpfs/projects/punim2657/sfs_main:/workspace \
  --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/TPS_Dataset_20250829/SfS_pp:/Dataset/SfS_pp" \
  /data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif \
  /workspace/sfspreproc-docker/Hierarchy-Clear 8
```

**Key Change**: Direct dataset binding to clean TPS dataset location eliminates file copying and ensures data integrity.

## Key Files and Locations

### Fixed Source Files
- **Segmentation fault fix**: `/data/gpfs/projects/punim2657/sfs_main/sfs_modified_src/class/ranking_system.cpp` (lines 954-968)
- **Main program**: `/data/gpfs/projects/punim2657/sfs_main/sfs_modified_src/main_headless_correct.cpp`

### Built Binary Location
- **Fixed binary**: `/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Hierarchy-Clear`

### Container and Dataset
- **Container**: `/data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif`
- **Clean TPS Dataset**: `/data/gpfs/projects/punim2657/sfs_preprocessing/TPS_Dataset_20250829/SfS_pp/`
- **Legacy Dataset Location**: `/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/` (may contain mixed/contaminated data)

### TPS Preprocessing Tools
- **NURBS-compatible generator**: `/data/gpfs/projects/punim2657/sfs_main/nurbs_compatible_tps_generator`
- **Surface_F generator**: `/data/gpfs/projects/punim2657/sfs_main/generate_nurbs_compatible_surface_f`
- **Complete preprocessing script**: `/data/gpfs/projects/punim2657/sfs_main/generate_complete_tps_preprocessing.sh`

## Status of Fixes Applied

✅ **Segmentation fault fixed**: Array bounds checking in SortRoot function
✅ **NURBS-compatible TPS**: Preserved original normal vectors with TPS Z-coordinates  
✅ **7-field PCD format**: Surface_F and Breaklines use proper header format
✅ **Fixed binary built**: Successfully compiled with segmentation fault fix

## Current Issue

The pipeline runs without crashing but reports "Total number : 0" for feature matching, suggesting TPS surface characteristics may still differ from what the feature matching algorithm expects. The core segmentation fault has been resolved.

## 🏭 TPS Preprocessing Pipeline (Optional)

For generating your own pottery data from raw meshes:

```bash
cd /data/gpfs/projects/punim2657/sfs_preprocessing/

# Complete TPS preprocessing pipeline (recommended)
./run_complete_tps_preprocessing.sh  # Includes all steps with SLURM

# OR run individual steps:

# 1-3: Mesh processing, surface generation, Surface_F
./run_mesh_processing.sh A 8  # If this script exists

# 4: Breakline generation ⚠️ REQUIRES SLURM (computationally intensive)
sbatch run_edgeline_extraction.sbatch  # ~12-15 minutes for 8 pieces

# 5: Axis extraction (parallel SLURM)
./submit_axis_jobs.sh  # MATLAB PotSAC processing

# 6: Auto-organize into clean dataset
./organize_dataset.sh
```

**⚠️ IMPORTANT NOTES**:
- **EdgeLine extraction must use SLURM** (interactive sessions timeout)
- **Processing time**: ~90 seconds per pottery piece, ~15 minutes total
- **Use SLURM bind mounting**: No file copying required - bind mount clean dataset directly
- **Dependencies**: Must run in container environment

## Quick Test Command

To quickly test if the fix is working with clean TPS dataset:
```bash
# Should see "SortRoot: Processing X LCS entries for 8 shards" without segmentation fault
CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"
$CONTAINER_PATH exec \
  --bind /data/gpfs/projects/punim2657/sfs_main:/workspace \
  --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/TPS_Dataset_20250829/SfS_pp:/Dataset/SfS_pp" \
  /data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif \
  /workspace/sfspreproc-docker/Hierarchy-Clear 8 2>&1 | head -30
```
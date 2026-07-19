# TPS Preprocessing Pipeline Documentation

## Overview
This document describes the TPS (Thin Plate Splines) preprocessing pipeline for pottery shape-from-shading reconstruction.

## Pipeline Output Locations

**Base Directory**: `/data/gpfs/projects/punim2657/sfs_preprocessing/`

### Step-by-Step Output Mapping:

1. **Mesh-to-Point Conversion**
   - **Location**: `Temp/Data/Pot_A/`
   - **Output**: `Pot_A_Piece_XX_Point.pcd` (mesh vertices as point cloud)
   - **Example**: `Pot_A_Piece_01_Point.pcd` (41,197 points, 494KB)

2. **TPS Surface Generation**
   - **Location**: `Surfaces/` (root level)
   - **Output**: `Pot_A_Piece_XX_Surface_0.xyz`, `Pot_A_Piece_XX_Surface_1.xyz`
   - **Content**: TPS-fitted surfaces with normals (X Y Z Nx Ny Nz format)
   - **Also in**: `Temp/Data/Pot_A/` (duplicate copies)

3. **Surface_F Generation (TPS Feature Surfaces)**
   - **Location**: `TPS_Surface_F_XYZ/`
   - **Output**: `Pot_A_Piece_XX_Surface_F.xyz`
   - **Also**: Root level `Pot_A_Piece_XX_Surface_F.pcd`
   - **Content**: Feature-compatible surface representation

4. **Axis Extraction (MATLAB PotSAC)**
   - **Location**: `TPS_Output/Axes/`
   - **Output**: `Pot_A_Piece_XX_Axis.xyz`
   - **Format**: Position Direction (6 values per line)
   - **Generated via**: SLURM parallel processing

5. **Breakline Generation (EdgeLine Extraction)**
   - **Location**: Root level (generated files) → moved to `TPS_Dataset_YYYYMMDD/SfS_pp/Breaklines/`
   - **Output**: `Pot_A_Piece_XX_Breakline_0.pcd`, `Pot_A_Piece_XX_Breakline_1.pcd`
   - **Generation Method**: Container-based edgeline extraction
   - **SLURM Required**: Process is computationally intensive (~90 seconds per piece)

## SLURM-Based Dataset Loading (RECOMMENDED)

**No file copying required** - Use clean TPS dataset directly via bind mounting:

```bash
# SLURM method (Recommended)
sbatch test_clean_tps_relaxed.sbatch

# Manual container method
CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"
$CONTAINER_PATH exec \
  --bind /data/gpfs/projects/punim2657/sfs_main:/workspace \
  --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/TPS_Dataset_20250829/SfS_pp:/Dataset/SfS_pp" \
  sfspreproc.sif /workspace/sfspreproc-docker/Hierarchy-Clear 8
```

**Benefits**:
- ✅ **No file copying** - eliminates contamination risk
- ✅ **Uses clean TPS dataset** - properly formatted breaklines
- ✅ **Data integrity** - source dataset remains unchanged
- ✅ **Efficient** - immediate dataset access via bind mount

## Automated Dataset Organization

**Script**: `/data/gpfs/projects/punim2657/sfs_preprocessing/organize_dataset.sh`

**Auto-Organization Features**:
- Creates dated dataset directories: `TPS_Dataset_YYYYMMDD/`
- Maintains original sample structure (`SfS_pp/` subdirs)
- Copies all TPS components automatically
- Generates comprehensive documentation
- Creates `latest_tps_dataset` symlink
- Validates file inventory

**Usage**:
```bash
cd /data/gpfs/projects/punim2657/sfs_preprocessing/
./organize_dataset.sh
```

**Integration**: Add to end of preprocessing pipeline scripts to auto-organize outputs

## Breakline Generation Process

**Script**: `/data/gpfs/projects/punim2657/sfs_preprocessing/run_edgeline_extraction.sbatch`

**Requirements**:
- Must run in container environment (dependency issues on host)
- Requires SLURM submission due to processing time (~12-15 minutes for 8 pieces)
- Uses overlay mount for temporary file access

**Command**:
```bash
cd /data/gpfs/projects/punim2657/sfs_preprocessing/
sbatch run_edgeline_extraction.sbatch  # Processes all 8 pieces
```

**Process Flow**:
1. EdgeLine extraction runs: `./build/edgeline_extraction A 8`
2. Generates breakline PCD files and Surface_F files
3. Files automatically moved to organized dataset structure
4. Updates dataset documentation to reflect completion

**Output**: Complete TPS dataset with all required components for SFS integration

### Previous Issue Resolved:
**TPS Breaklines Missing**: Now resolved through SLURM-based edgeline extraction. Process generates TPS-compatible breaklines from mesh geometry, eliminating contamination with NURBS breaklines.

## SLURM-Based Parallel Axis Extraction

### Problem Solved
The MATLAB PotSAC axis extraction algorithm takes 2-3 minutes per piece, making sequential processing of 8 pieces take ~20 minutes with timeout issues.

### Solution: SLURM Job Arrays
**Location**: `/data/gpfs/projects/punim2657/sfs_preprocessing/`

**Files Created**:
- `extract_single_axis.m` - Parameterized MATLAB function for single piece extraction
- `slurm_axis_extraction.sh` - SLURM job array script (1-8 pieces)
- `submit_axis_jobs.sh` - Job submission script with monitoring
- `check_axis_results.sh` - Results verification script

**Usage**:
```bash
cd /data/gpfs/projects/punim2657/sfs_preprocessing/
./submit_axis_jobs.sh           # Submit all 8 pieces in parallel
squeue -u $USER                 # Monitor job progress
./check_axis_results.sh         # Verify completion and copy to main dataset
```

**Benefits**:
- ✅ **Parallel execution**: All 8 pieces run simultaneously
- ✅ **No timeout issues**: Each job allocated proper time/resources
- ✅ **Fault tolerance**: Individual job failures don't affect others
- ✅ **Resource efficiency**: Proper CPU/memory allocation per piece
- ✅ **Monitoring**: Individual job logs for debugging
- ✅ **Automatic validation**: Results copied to main SFS dataset

**SLURM Configuration**:
- Job time: 10 minutes per piece (vs 20 minutes sequential)
- Memory: 4GB per job
- CPU: 1 core per piece
- Array: 1-8 (pieces)

**PotSAC Algorithm Fix Included**:
- Automatic detection of inconsistent data structure returns
- Position/direction vector identification using norm analysis
- Sample-compatible output format (position first, then direction)

### Command Documentation
**Remember these commands for axis extraction**:
```bash
# Parallel SLURM approach (RECOMMENDED)
./submit_axis_jobs.sh && sleep 30 && ./check_axis_results.sh

# Sequential MATLAB approach (FALLBACK only)
module load MATLAB/2024b_Update_3 && matlab -batch "extract_all_tps_axes"
```
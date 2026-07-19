# Data Locations and Reference Datasets

## Original Sample Data Location

**Location**: `/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/`

**Downloaded from**: Google Drive ID `1o1FM0ZijjhtWY_7FV2qINLJoAOaUF--8` (found in download.sh)

**Contents**:
- `Surfaces/` - Original NURBS-based surface files (Nov 2024)
  - Pot_A_Piece_01-08_Surface_0.xyz
  - Pot_A_Piece_01-08_Surface_1.xyz
  - All other pots (B through J) with their respective pieces
- `Axes/` - Original axis files for all pottery pieces
- `Breaklines/` - Original breakline PCD files for all pieces
- `Ground Truth/` - Ground truth transformation matrices
- `Lighting/` - Lighting condition files for SFS reconstruction
- `Mesh/` - Original mesh files for all pottery pieces

**Usage**: This is the authoritative reference dataset for comparing TPS-generated surfaces against original NURBS surfaces. Always use this location when comparing "sample vs generated" data.

**Key Dates**:
- Original sample surfaces: Nov 20, 2024
- TPS generated surfaces: Aug 29, 2025
- Repository download: Available via gdown command in download.sh files

## TPS Pipeline Data Locations

**Main Processing Directory**: `/data/gpfs/projects/punim2657/sfs_preprocessing/`

### Dataset Organization
- `NURBS_Dataset_YYYYMMDD/` - Timestamped dataset folders
- `Temp/Data/Pot_A/` - Temporary processing files
- `build/` - Compiled binaries and executables

### Key Output Files
- Surface files: `Pot_A_Piece_*_Surface_*.xyz`
- Breaklines: `Pot_A_Piece_*_Breaklines.pcd`
- Axes: `Pot_A_Piece_*_Axis.txt`

## SFS Main Pipeline Data Locations

**Main Processing Directory**: `/data/gpfs/projects/punim2657/sfs_main/`

### Results Organization
- `results_YYYY_MM_DD_HHMM/` - Timestamped result folders
- `assembly_data.txt` - Assembly transformation matrices
- `piece_*_surface.xyz` - Transformed surface point clouds
- `ply_output/` - PLY visualization files

### Container and Build
- `sfspreproc-docker/` - Main processing container
- `pcl_191_nurbs.sif` - Singularity container image

## Ground Truth Data Usage

**CRITICAL PRINCIPLE**: Ground truth data is **ONLY for verification** - never part of the algorithmic implementation.

**Location**: Various `Ground Truth/` subdirectories in sample data

**Usage**:
- ✅ **Correct**: Compare results against ground truth for validation
- ✅ **Correct**: Analyze connection coverage and accuracy
- ❌ **Wrong**: Hardcode ground truth adjacency matrices in algorithms
- ❌ **Wrong**: Use ground truth to guide assembly decisions

## File Naming Conventions

### Surface Files
- `Pot_A_Piece_01_Surface_0.xyz` - Base surface (NURBS/TPS generated)
- `Pot_A_Piece_01_Surface_F.xyz` - Final processed surface (after edge extraction)

### Assembly Results
- `piece_1_surface.xyz` - Transformed surface points for piece 1
- `pot_a_complete_assembly.ply` - Combined assembly visualization

### Transformation Data
- `assembly_data.txt` - Human-readable transformation matrices
- Rotation matrices: `[R11,R12,R13;R21,R22,R23;R31,R32,R33]`
- Translation vectors: `[tx,ty,tz]`

## Processing Pipeline Flow

```
Original Samples → TPS/NURBS Preprocessing → SFS Reconstruction → Assembly Results
     ↓                      ↓                        ↓                   ↓
Ground Truth        Generated Surfaces        Transformation      PLY Visualization
(verification)      (algorithm input)          Matrices           (final output)
```

## Storage Management

- Remove old binaries after building new ones
- Use timestamped folders to avoid conflicts
- Document build procedures in BUILD_COMMANDS.md
- Record pipeline execution commands for reproducibility
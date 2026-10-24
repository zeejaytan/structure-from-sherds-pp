# NURBS Fix Results - Complete Failure Analysis

## Test Date: November 3-4, 2025

## Dataset Comparison

### 1. Sample Dataset (Baseline - 100% Success)
**Location**: `/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/`
**Source**: Original GitHub repository (verified working)
**Results**:
- **Pieces Loaded**: 8/8 (100%)
- **Pieces Matched**: 8/8 (100%)
- **Edges Found**: 15/15 (100%)
- **Status**: ✅ PERFECT BASELINE

### 2. Failed NURBS Dataset (September 7, 2025)
**Location**: `/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_CompleteFixed/`
**Source**: Preprocessing with **INCORRECT TPS normals**
**Code Issue**: Used `computeSurfaceNormal()` (TPS derivatives) instead of `fit.m_nurbs.EvNormal()`
**Results**:
- **Pieces Loaded**: 8/8 (100%)
- **Pieces Matched**: 0/8 (0%)
- **Edges Found**: 0/0 (0%)
- **Normal Rejections**: ~6,339 rejections (14x more than sample)
- **Status**: ❌ COMPLETE FAILURE (TPS normals)

### 3. "Fixed" NURBS Dataset (November 3, 2025)
**Location**: `/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20251103/`
**Source**: Preprocessing with **"CORRECT" NURBS normals**
**Code Used**: `fit.m_nurbs.EvNormal()` at line 902 in `mesh_processing_headless.cpp`
**Executable**: `original_nurbs_preprocessing/build/MeshPreprocessingHeadless` (built Oct 24, 2025)
**Results**:
- **Pieces Loaded**: 6/8 (75%) - **Pieces 06 and 08 EXCLUDED**
- **Pieces Matched**: 0/6 (0%)
- **Edges Found**: 0/9 (0%)
- **Processing Time**: ~4 minutes
- **Status**: ❌ COMPLETE FAILURE (even with "correct" code)

## Critical Findings

### Issue 1: Breakline Extraction Failure
**Pieces 06 and 08 excluded due to insufficient breakline points:**
- Piece 06 Breakline_0: Only 30 points (< 50 threshold)
- Piece 08 Breakline_0: Only 30 points (< 50 threshold)
- Assembly code requires >= 50 breakline points to load a piece
- **Root Cause**: Breakline extraction failed during preprocessing

### Issue 2: Complete Assembly Failure (0% Success)
**Even with 6 valid pieces, assembly completely failed:**
- 0/6 pieces matched (0% accuracy)
- 0/9 edges found (0% accuracy)
- Expected: 60-80% with correct NURBS normals
- Actual: Same 0% failure as TPS normal dataset

## Comparison with Sample Dataset

### Sample Dataset Structure (Working):
```
original_samples/SfS_pp/
├── Axes/                    # 8 axis files
├── Breaklines/              # 16 breakline files (2 per piece)
├── Surfaces/                # 16 surface files (2 per piece, NURBS normals)
├── Mesh/                    # 8 mesh files
├── Ground Truth/            # 9 transformation/graph files
└── Ground Truth Axes/       # 8 axis files
```

### Fixed NURBS Dataset Structure (Failing):
```
NURBS_Dataset_20251103/SfS_pp/
├── Axes/                    # 8 axis files (+ symlinks)
├── Breaklines/              # 16 breakline files (2 per piece, but 06/08 Breakline_0 inadequate)
├── Surfaces/                # 16 surface files (2 per piece, NURBS normals)
├── Mesh/                    # 8 mesh files
├── Ground Truth/            # 9 transformation/graph files (copied from sample)
└── Ground Truth Axes/       # 8 axis files (copied from sample)
```

## Surface File Format Verification

### Sample Dataset Surface Format (Working):
```
52074100  3831470  387711000  -0.313742  -0.37257  -0.87336
67403000 -19528000  391428000  -0.315528 -0.310689 -0.896613
```
Format: `x y z nx ny nz` (6 columns)

### Fixed NURBS Surface Format (Failing):
```
52074100  3831470  387711000  -0.313742  -0.37257  -0.87336
67403000 -19528000  391428000  -0.315528 -0.310689 -0.896613
```
Format: `x y z nx ny nz` (6 columns) - **IDENTICAL TO SAMPLE**

## Conclusion

### The NURBS "Fix" Did Not Work

Despite using the "correct" NURBS normal computation code (`fit.m_nurbs.EvNormal()`), the assembly still completely failed with 0% accuracy.

### Possible Root Causes:

1. **Wrong Executable Used**: The preprocessing may have used the wrong executable (TPS version) despite checking for the correct code
2. **NURBS Fitting Parameters Wrong**: The NURBS surface fitting parameters may be incorrect (degree, control points, etc.)
3. **Coordinate System Issues**: The normals may be in the wrong coordinate system or orientation
4. **Breakline Extraction Bugs**: The preprocessing has additional bugs beyond just normal computation
5. **Sample Dataset is Not NURBS**: The working sample dataset may not actually use NURBS normals at all

### Next Steps Required:

1. **Verify executable used**: Confirm which executable actually ran during preprocessing
2. **Compare sample vs NURBS surface files directly**: Byte-by-byte comparison of a known good piece
3. **Check NURBS fitting parameters**: Review degree, knot vectors, control point density
4. **Investigate breakline extraction**: Why did pieces 06 and 08 fail?
5. **Consider alternative hypothesis**: Sample dataset may use a completely different preprocessing method

### Key Question:

**If the "correct" NURBS normal code produces 0% accuracy, but the sample achieves 100%, what is the sample dataset actually using?**

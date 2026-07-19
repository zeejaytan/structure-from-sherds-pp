# TPS LCS Feature Matching Optimization

## Problem Analysis

### Issue Identified
TPS surfaces were generating excessive feature matches compared to NURBS:
- **NURBS baseline**: 124 initial matches → 70 after pruning (successful)
- **TPS surfaces**: 4492 initial matches → system timeout during pruning (failed)

### Root Cause
TPS surfaces are more geometrically uniform than NURBS, causing the feature matching algorithm to find 36x more potential correspondences. The fixed feature matching thresholds were too permissive for TPS characteristics.

## Solution Implementation

### File Modified
- **Location**: `/data/gpfs/projects/punim2657/sfs_main/sfs_modified_src/class/feature_matching.cpp`
- **Function**: `FeatureComp()` - lines 1444-1450
- **Purpose**: Tighten feature matching thresholds to reduce initial match generation

### Changes Made

#### Before (Permissive Thresholds)
```cpp
else {
    Q_size.resize(4);
    Q_size[0] = 0.15;	// D
    Q_size[1] = 0.15;	// H
    Q_size[2] = 0.15;	// Theta
    Q_size[3] = 0.2;
}
```

#### After (Stricter Thresholds)
```cpp
else {
    Q_size.resize(4);
    // Stricter thresholds for TPS surfaces to reduce excessive matches (was 4492, target ~100-200)
    Q_size[0] = 0.08;	// D (distance) - reduced from 0.15 
    Q_size[1] = 0.08;	// H (height) - reduced from 0.15
    Q_size[2] = 0.08;	// Theta (angle) - reduced from 0.15
    Q_size[3] = 0.10;	// Additional feature - reduced from 0.2
}
```

### Parameter Analysis

| Parameter | Feature Type | Original | New | Reduction |
|-----------|-------------|----------|-----|-----------|
| Q_size[0] | Distance (D) | 0.15 | 0.08 | 47% |
| Q_size[1] | Height (H) | 0.15 | 0.08 | 47% |
| Q_size[2] | Angle (Theta) | 0.15 | 0.08 | 47% |
| Q_size[3] | Additional | 0.20 | 0.10 | 50% |

## Technical Background

### Feature Matching Process
1. **Feature Extraction**: Each piece generates geometric features (distance, height, angle)
2. **Quantization**: Features are discretized using `Q_size` thresholds
3. **LCS Algorithm**: Longest Common Subsequence finds matching feature patterns
4. **Initial Matches**: Generates candidate correspondences for pairwise evaluation

### Impact of Thresholds
- **Smaller thresholds** = More selective matching = Fewer initial matches
- **Larger thresholds** = More permissive matching = More initial matches

### TPS vs NURBS Characteristics
- **TPS surfaces**: More uniform curvature → similar features across pieces
- **NURBS surfaces**: More varied curvature → more distinct features per piece

## Expected Results

### Target Match Counts
- **Previous**: 4492 initial matches (caused system timeout)
- **Target**: ~100-200 initial matches (similar to NURBS baseline)
- **After pruning**: ~70-100 final matches for optimization

### Performance Impact
- **Reduced computational load** in pairwise pruning phase
- **Faster processing** through LCS entry handling
- **Successful completion** of assembly optimization

## Deployment

### Build Process
```bash
# Copy modified code
cp /data/gpfs/projects/punim2657/sfs_main/sfs_modified_src/class/feature_matching.cpp \
   /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/class/

# Rebuild binary
cd /data/gpfs/projects/punim2657/sfs_main
CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"
$CONTAINER_PATH exec --bind $PWD:/workspace sfspreproc.sif \
  /bin/bash -c "cd /workspace/sfspreproc-docker && make clean && make -j4"
```

### Testing Command
```bash
sbatch test_clean_tps_relaxed.sbatch
```

## Validation Criteria

### Success Metrics
1. **Initial match count**: Reduced from 4492 to ~100-200
2. **Pairwise pruning**: Completes without timeout
3. **LCS processing**: Successfully processes entries
4. **Optimization**: Runs to completion without matrix errors
5. **Final assembly**: Generates valid reconstruction states

### Monitoring Points
- Feature matching output: `Total number : X`
- Pruning completion: `Total number pruned: Y`
- LCS processing: `SortRoot: Processing Z LCS entries`

## Rollback Plan

If results are unsatisfactory (too few matches), can incrementally increase thresholds:

```cpp
Q_size[0] = 0.10;	// Moderate increase
Q_size[1] = 0.10;	// Moderate increase
Q_size[2] = 0.10;	// Moderate increase  
Q_size[3] = 0.12;	// Moderate increase
```

## Related Documentation

- **Main Pipeline Guide**: `/data/gpfs/projects/punim2657/sfs_preprocessing/COMPLETE_PIPELINE_GUIDE.md`
- **Build Instructions**: `/data/gpfs/projects/punim2657/sfs_main/BUILD_AND_RUN_COMMANDS.md`
- **Technical Notes**: `/data/gpfs/projects/punim2657/sfs_main/CLAUDE.md`

---

**Change Date**: August 29, 2025  
**Change ID**: TPS-LCS-OPT-001  
**Status**: Testing in progress (Job 14930084)
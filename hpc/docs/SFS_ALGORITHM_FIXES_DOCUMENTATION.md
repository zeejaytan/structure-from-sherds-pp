# SFS Pottery Reconstruction Algorithm Fixes

**Date**: September 6, 2025  
**Issue**: Pottery pieces misplaced and overlapping in assembly  
**Root Cause**: Algorithmic parameters too permissive for pottery reconstruction

## Problem Analysis

The SFS reconstruction was producing incorrect assemblies with:
- **Major misplacements**: Blue, Green, Cyan pieces isolated from main assembly
- **Minor overlaps**: Yellow, Magenta, Purple pieces occupying same space
- **Assembly score**: 340 but geometrically incorrect

## Algorithmic Issues Identified

### 1. Outlier Rejection Too Permissive
**Location**: `class/reconstruction.cpp:587`
```cpp
// ORIGINAL (problematic)
RejectOutlier(cor_line, 20, 0.7);  // 20mm distance, 0.7 normal threshold
```
**Problem**: 20mm distance threshold allows incorrect correspondences for pottery

### 2. Insufficient Correspondence Requirements  
**Location**: `class/reconstruction.cpp:593`
```cpp
// ORIGINAL (problematic)
if (cor_line.cor.size() > MINIMUM_NUMBER / 2) {  // Only 3 points required
```
**Problem**: 3 correspondence points insufficient for reliable pottery assembly

### 3. Loose Convergence Criteria
**Location**: `class/reconstruction.cpp:1861`
```cpp
// ORIGINAL (problematic)
if (isConverge(T, 0.05, 1.0))  // 1mm translation tolerance
```
**Problem**: 1mm tolerance too loose for precision pottery reconstruction

### 4. Weak Optimization Constraints
**Location**: `class/reconstruction.cpp:1733-1737`
```cpp
// ORIGINAL (problematic)
double w_d(1.0), w_n(3.0), w_line(1.0);  // Weak constraint weights
double w_a(0.1);                           // Very weak axis weight
int max_iteration = 100, ceres_iteration = 100;  // Limited iterations
```
**Problem**: Weak constraints allow poor local minima

### 5. Permissive Loss Functions
**Location**: `class/reconstruction.cpp:1783-1786`
```cpp
// ORIGINAL (problematic)
ceres::LossFunction* loss_dist = new ceres::CauchyLoss(5.0);  // Too permissive
ceres::LossFunction* loss_norm = new ceres::CauchyLoss(2.0);  // Too permissive
```

## Applied Fixes

### 1. ✅ FIXED: Stricter Outlier Rejection
```cpp
// FIXED VERSION
RejectOutlier(cor_line, 2.0, 0.85);  // FIXED: Much stricter outlier rejection for pottery
```
**Change**: Distance threshold 20mm → 2mm, Normal threshold 0.7 → 0.85
**Impact**: Eliminates spurious correspondences between distant pieces

### 2. ✅ FIXED: Higher Correspondence Requirements
```cpp
// FIXED VERSION  
if (cor_line.cor.size() > 8) {  // FIXED: Require more correspondences for reliable pottery assembly
```
**Change**: Minimum correspondences 3 → 8 points
**Impact**: Only allows connections with strong geometric evidence

### 3. ✅ FIXED: Tighter Convergence Criteria
```cpp
// FIXED VERSION
if (isConverge(T, 0.02, 0.2))  // FIXED: Stricter convergence for pottery assembly
```
**Change**: Rotation 0.05 → 0.02 rad, Translation 1.0 → 0.2mm  
**Impact**: Forces more precise alignment

### 4. ✅ FIXED: Stronger Optimization Constraints
```cpp
// FIXED VERSION
double w_d(2.0), w_n(5.0), w_line(3.0);  // FIXED: Increased constraint weights
double w_a(1.0);  // FIXED: Increased axis weight from 0.1 to 1.0
int max_iteration = 200, ceres_iteration = 200;  // FIXED: More iterations
```
**Change**: All weights increased, iterations doubled
**Impact**: Better convergence to global minimum

### 5. ✅ FIXED: Restrictive Loss Functions
```cpp
// FIXED VERSION
ceres::LossFunction* loss_dist = new ceres::CauchyLoss(1.0);   // FIXED: Stricter distance constraint
ceres::LossFunction* loss_norm = new ceres::CauchyLoss(0.5);   // FIXED: Stricter normal constraint
ceres::LossFunction* loss_axis = new ceres::CauchyLoss(1.0);   // FIXED: Stricter axis constraint
ceres::LossFunction* loss_rim = new ceres::CauchyLoss(1.0);    // FIXED: Stricter rim constraint
```

### 6. ✅ FIXED: Tighter Optimization Tolerance
```cpp
// FIXED VERSION
options.function_tolerance = 1.0e-6;  // FIXED: Tighter tolerance than default 1.0e-3
```

## How to Revert Changes

### Option 1: Git-Based Reversion (Recommended)
```bash
cd /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
git checkout HEAD~1 class/reconstruction.cpp  # Revert to previous version
make clean && make -j4  # Rebuild with original parameters
```

### Option 2: Manual Parameter Reversion
Edit `/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/class/reconstruction.cpp`:

1. **Line 587**: Change back to `RejectOutlier(cor_line, 20, 0.7);`
2. **Line 593**: Change back to `if (cor_line.cor.size() > MINIMUM_NUMBER / 2) {`  
3. **Line 1861**: Change back to `if (isConverge(T, 0.05, 1.0))`
4. **Lines 1733-1737**: Restore original weights:
   ```cpp
   double w_d(1.0), w_n(3.0), w_line(1.0);
   double w_a(0.1);
   int max_iteration = 100, ceres_iteration = 100;
   ```
5. **Lines 1783-1786**: Restore original loss functions:
   ```cpp
   ceres::LossFunction* loss_dist = new ceres::CauchyLoss(5.0);
   ceres::LossFunction* loss_norm = new ceres::CauchyLoss(2.0);
   ceres::LossFunction* loss_axis = new ceres::CauchyLoss(2.0);
   ceres::LossFunction* loss_rim = new ceres::CauchyLoss(2.0);
   ```
6. **All function_tolerance lines**: Change back to `options.function_tolerance = CERES_FUNC_TOL;`

### Option 3: Parameter Configuration File (Future Enhancement)
Create `sfs_config.h` with toggleable parameters:
```cpp
// For easy switching between original and fixed parameters
#define USE_POTTERY_OPTIMIZED_PARAMS 1  // Set to 0 to use original

#if USE_POTTERY_OPTIMIZED_PARAMS
    #define OUTLIER_DIST_THRESHOLD 2.0
    #define OUTLIER_ANGLE_THRESHOLD 0.85
    #define MIN_CORRESPONDENCES 8
    // ... other optimized parameters
#else
    #define OUTLIER_DIST_THRESHOLD 20.0
    #define OUTLIER_ANGLE_THRESHOLD 0.7  
    #define MIN_CORRESPONDENCES 3
    // ... original parameters
#endif
```

## Binary Information

**Fixed Binary Location**: `/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Hierarchy-Clear`
**Build Date**: September 6, 2025
**Compilation**: Successful with warnings (non-critical)

## Testing Protocol

### Before Testing Original Parameters
```bash
# Backup current fixed binary
cp Hierarchy-Clear Hierarchy-Clear-FIXED

# Test with original dataset
sbatch test_sfs_fixed_algorithm.sbatch
```

### Testing Fixed Algorithm
```bash
# Use the newly built binary with fixes
sbatch run_fixed_sfs_test.sbatch
```

## Expected Improvements

1. **Geometric Validity**: Pieces should be properly positioned without overlaps
2. **Assembly Connectivity**: Blue, Green, Cyan pieces should connect to main assembly
3. **Spatial Relationships**: All pieces should have realistic gaps (0-5mm)
4. **Convergence**: Better optimization convergence with fewer iterations
5. **Correspondence Quality**: Only high-confidence piece connections

## Notes

- **Computational Impact**: Fixed algorithm may take 10-20% longer due to stricter criteria
- **Success Rate**: May reduce assembly success rate for poor quality data (intended)
- **Pottery-Specific**: These parameters are optimized for pottery fragment reconstruction
- **Reversible**: All changes can be easily reverted using documented methods above

## Validation Required

1. Test on complete Pot A dataset (8 pieces)
2. Compare assembly quality visually in MeshLab
3. Verify no overlapping geometry
4. Confirm proper piece connectivity
5. Measure improvement in breakline contact analysis

---

**Status**: Algorithm fixes implemented and documented for easy reversion
**Next Step**: Test fixed algorithm on pottery reconstruction
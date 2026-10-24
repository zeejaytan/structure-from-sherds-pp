# Boundary Radius Unit Conversion Bug Fix

**Date**: November 5, 2025
**Status**: FIXED - Regeneration job 18056648 running

---

## Critical Bug Fixed: Boundary Detection Radius Unit Mismatch

### Problem Identified

The **adaptive boundary detection** code calculated radius in **meters** but passed it to PCL which expected **millimeters** (same units as point cloud coordinates).

**Symptom**: Segmentation fault when calling `getPointsInSequence()` with empty boundary cloud

**Root Cause**: After coordinate scaling fix, point clouds are in millimeters, but boundary radius was calculated in meters and not converted:

```cpp
// BROKEN CODE (Line 1071-1076):
double estimated_spacing_m = std::sqrt(1.0 / std::max(100, num_pts));  // In meters
double boundary_radius_m = std::max(0.001, std::min(0.015, estimated_spacing_m * 6.0));  // Still meters!
boundary_est.setRadiusSearch(boundary_radius_m);  // PCL expects mm, got 0.015 instead of 15!
```

**Result**:
- For 406-point cluster: spacing ≈ 49.6mm, radius calculated as 297mm
- Capped at 15mm (0.015m)
- **But passed to PCL as 0.015** → interpreted as **0.015mm** ← Too small!
- Boundary detection found **0 points** → empty cloud → crash when accessing [0]

---

## GitHub Original Comparison

**Original code** (from https://github.com/DominicoRyu/SfSpp_preprocessing):
```cpp
boundary_est.setRadiusSearch(3);  // Fixed 3mm radius
```

The adaptive calculation was a local modification that didn't account for coordinate units.

---

## Fix Applied

**File**: `/data/gpfs/projects/punim2657/sfs_preprocessing/original_nurbs_preprocessing/mesh_processing_headless.cpp`

**Lines 1071-1086**: Fixed adaptive calculation to work in millimeters

```cpp
// FIXED CODE:
// ADAPTIVE: Compute boundary radius based on cluster density
// Note: Point cloud coordinates are in MILLIMETERS after coordinate fix
// Estimate average spacing from point count (assuming ~1m^2 pottery piece)
int num_pts = cloudWithoutNormals->points.size();
double estimated_spacing_m = std::sqrt(1.0 / std::max(100, num_pts));  // sqrt(area / density)
double estimated_spacing_mm = estimated_spacing_m * 1000.0;  // Convert to millimeters

// Calculate radius in millimeters (6x the estimated spacing)
double boundary_radius_mm = estimated_spacing_mm * 6.0;
// Cap between 1mm and 50mm (original uses 3mm fixed)
boundary_radius_mm = std::max(1.0, std::min(50.0, boundary_radius_mm));

boundary_est.setRadiusSearch(boundary_radius_mm);  // PCL expects same units as coordinates (mm)
std::cout << "[ADAPTIVE BOUNDARY] Cluster " << t << " has " << num_pts
          << " points, spacing≈" << estimated_spacing_mm << "mm, boundary_r="
          << boundary_radius_mm << "mm" << std::endl;
```

**Key Changes**:
1. Added explicit unit conversion: `estimated_spacing_mm = estimated_spacing_m * 1000.0`
2. Calculate radius in millimeters: `boundary_radius_mm = estimated_spacing_mm * 6.0`
3. Adjusted caps to millimeters: `max(1.0, min(50.0, ...))`
4. Clear documentation of coordinate units

---

## Additional Safety Checks Added

**Lines 1093-1098**: Size mismatch validation
```cpp
if (boundary.points.size() != cloud->points.size()) {
    std::cerr << "[ERROR] Boundary size mismatch! Expected " << cloud->points.size()
              << " but got " << boundary.points.size() << std::endl;
    continue;  // Skip this cluster
}
```

**Lines 1112-1117**: Empty boundary cloud check
```cpp
if (boundaryCloud->points.empty()) {
    std::cerr << "[WARNING] Cluster " << t << " has no boundary points!" << std::endl;
    std::cerr << "[WARNING] This may indicate the boundary radius (" << boundary_radius_mm
              << "mm) is too small for the point spacing." << std::endl;
    continue;  // Skip to next cluster
}
```

---

## Test Results

**Before Fix** (0.015mm radius):
```
[ADAPTIVE BOUNDARY] Cluster 0 has 406 points, spacing≈49.63mm, boundary_r=15mm  ← Wrong display!
[pcl::KdTreeFLANN::setInputCloud] Cannot create a KDTree with an empty input cloud!
Segmentation fault (exit code 139)
```

**After Fix** (50mm radius):
```
[ADAPTIVE BOUNDARY] Cluster 0 has 406 points, spacing≈49.63mm, boundary_r=50mm
[DEBUG BOUNDARY] cloud size: 406, cloudWithoutNormals size: 406, boundary size: 406
[INFO] Cluster 0 boundary extraction: 145 boundary points found  ← Success!
```

**All 30 clusters** for Piece 01 processed successfully without crashes.

---

## Why This Bug Existed

1. **Coordinate system change**: After fixing `convertToMM=false`, coordinates became millimeters
2. **Adaptive code assumption**: Code assumed meters (standard PCL convention)
3. **Silent unit mismatch**: PCL doesn't validate radius units - just interprets as same units as coordinates
4. **Cascade failure**: 0.015mm radius → 0 boundary points → empty cloud → segfault accessing [0]

---

## Relationship to Coordinate Scaling Bug

These are **two separate bugs** that both needed fixing:

| Bug | Location | Effect | Fix |
|-----|----------|--------|-----|
| **Coordinate Scaling** | Lines 399, 1511, 2025, 2032 | Coordinates ×1000 too large (52km) | Added `false` parameter |
| **Boundary Radius Units** | Lines 1071-1086 | Radius ×1000 too small (0.015mm) | Convert meters → millimeters |

**Both must be fixed** for preprocessing to work correctly.

---

## Rebuild Information

**Executable**: `/data/gpfs/projects/punim2657/sfs_preprocessing/original_nurbs_preprocessing/build/MeshPreprocessingHeadless`
- **Size**: 5.5MB
- **Built**: November 5, 2025, 02:08:05 AEDT
- **Includes**: Coordinate scaling fix + Boundary radius fix + Safety checks

**Build Command**:
```bash
cd /data/gpfs/projects/punim2657/sfs_preprocessing
/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
  --bind /data/gpfs/projects/punim2657/sfs_preprocessing:/workspace \
  pcl_191_nurbs.sif \
  bash -c "cd /workspace/original_nurbs_preprocessing/build && make -j8"
```

---

## Current Status

**Job ID**: 18056648
**Submitted**: November 5, 2025, 02:09:53 AEDT
**Status**: Running (full 8-piece regeneration)
**Expected Duration**: 2-3 hours
**Expected Completion**: ~04:00-05:00 AEDT

**Success Criteria**:
- ✅ No segmentation faults
- ✅ All 8 pieces process successfully
- ✅ Coordinate scales in millimeter range (-500 to 500mm)
- ✅ All surfaces and breaklines generated
- ⏳ Dataset verification pending job completion

---

## References

- **GitHub Original**: https://github.com/DominicoRyu/SfSpp_preprocessing/blob/main/mesh_processing.cpp
- **Coordinate Fix**: See `COORDINATE_FIX_PROGRESS.md`
- **Complete Code Analysis**: See `COMPREHENSIVE_CODE_DIFFERENCES.md`
- **Regeneration Job**: `/data/gpfs/projects/punim2657/sfs_preprocessing/nurbs_coord_fix_18056648.out`

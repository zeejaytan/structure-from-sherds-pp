# NURBS Coordinate Scaling Bug Fix - Progress Report

**Date**: November 5, 2025
**Status**: Phase 2 Retry In Progress (TWO Critical Bugs Fixed)

---

## CRITICAL BUGS FIXED (TWO)

### **Bug #1: Coordinate Scaling (November 4)**
- **Root Cause**: `convertToMM=true` parameter multiplied already-millimeter coordinates by 1000
- **Impact**: Surface coordinates became 52,000,000mm (52km) instead of 32mm
- **Result**: 0% assembly accuracy (assembly system couldn't match pieces at wrong scale)

### **Bug #2: Boundary Radius Units (November 5)**
- **Root Cause**: Adaptive boundary radius calculated in meters but passed to PCL expecting millimeters
- **Impact**: Radius 0.015 interpreted as 0.015mm instead of 15mm → too small to find neighbors
- **Result**: Segmentation fault (boundary cloud empty → crash accessing [0])

### **Evidence**
```
Sample (working):     32.6657mm, -21.0367mm, 415.4600mm  ✓ Correct
Broken NURBS:  52,074,100mm, 3,831,470mm, ...            ✗ Wrong (1.6M times too large)
```

### **Fixes Applied**

**Bug #1 Fix** - Added `false` parameter at 4 locations (November 4):
1. **Line 399**: `writeMatrix_to_XYZ(pointCloudMatrix, fragmentMeshFile + "_Sampled.xyz", 3, false);`
2. **Line 1511**: `writeMatrix_to_XYZ(pointCloudMatrix, basePath + ".xyz", 6, false);`
3. **Line 2025**: `convertPLYtoXYZ(outputFileName0, dataPath + fileNameOnly + "_Surface_0.xyz", false);`
4. **Line 2032**: `convertPLYtoXYZ(outputFileName1, dataPath + fileNameOnly + "_Surface_1.xyz", false);`

**Bug #2 Fix** - Fixed boundary radius calculation (November 5, Lines 1071-1086):
```cpp
// Convert meters to millimeters before passing to PCL
double estimated_spacing_mm = estimated_spacing_m * 1000.0;
double boundary_radius_mm = estimated_spacing_mm * 6.0;
boundary_radius_mm = std::max(1.0, std::min(50.0, boundary_radius_mm));  // Cap 1-50mm
boundary_est.setRadiusSearch(boundary_radius_mm);  // Now in correct units!
```

See `BOUNDARY_RADIUS_FIX.md` for complete details on Bug #2.

---

## EXECUTION STATUS

### ✅ Phase 1: Critical Bug Fix (COMPLETE)

**Phase 1.1: Code Fix** ✅
- Fixed 4 locations in mesh_processing_headless.cpp
- Added `false` parameter to prevent ×1000 scaling
- Completed: Nov 4, 16:41

**Phase 1.2: Rebuild Executable** ✅
- Rebuilt MeshPreprocessingHeadless (5.5MB)
- Built EdgeLineExtractionHeadless (11MB)
- Completed: Nov 4, 16:42

**Phase 1.3: Initial Verification** ✅
- Single-piece test encountered path issues (non-critical)
- Proceeded directly to full regeneration
- Completed: Nov 4, 16:44

---

### ⏳ Phase 2: Full Dataset Regeneration (RETRY IN PROGRESS)

**Previous Attempt**:
- Job ID: 17906180 (FAILED - segmentation fault)
- Root cause: Boundary radius unit bug discovered

**Current Attempt**:
- Job ID: 18056648
- Submitted: Nov 5, 02:09
- Estimated Duration: 2-3 hours
- Expected Completion: ~04:00-05:00
- **Includes Both Fixes**: Coordinate scaling + Boundary radius units

**Processing Steps**:
1. ✓ Mesh preprocessing (with coordinate fix) - all 8 pieces
2. ⏳ MATLAB axis extraction
3. ⏳ Breakline extraction (EdgeLineExtractionHeadless)
4. ⏳ Dataset organization and verification

**Automatic Verification** (built into script):
- Coordinate scale check for all 8 pieces (expect < 1000mm)
- Breakline point count verification (especially pieces 06/08)
- Comparison with working reference dataset
- File count verification (16 surfaces, 16 breaklines, 8 axes)

**Output Location**:
```
/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_<timestamp>_CoordinateFix/
├── SfS_pp/
│   ├── Surfaces/    (16 .xyz files expected)
│   ├── Breaklines/  (16 .pcd files expected)
│   └── Axes/        (8 .xyz files expected)
```

**Monitoring**:
```bash
# Check job status
squeue -j 17906180

# View progress (once running)
tail -f nurbs_coord_fix_17906180.out
```

---

### ⏸️ Phase 3: SFS Assembly Test (READY)

**Script Prepared**: `test_coordinate_fixed_dataset.sbatch`

**When to Run**: After Phase 2 completes successfully

**What It Does**:
1. Finds most recent coordinate-fixed dataset
2. Verifies dataset completeness (16+16+8 files)
3. Updates data_path.h to point to new dataset
4. Rebuilds SFS executables
5. Runs assembly test
6. Compares results vs broken dataset (0% accuracy baseline)
7. Restores original data_path.h

**Execute When Ready**:
```bash
cd /data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp
sbatch test_coordinate_fixed_dataset.sbatch
```

**Expected Results** (if fix successful):
- Pieces loaded: 8/8 (vs broken: 6/8)
- Edges found: >60 (vs broken: 47)
- After pruning: >30 (vs broken: 8)
- Assembly accuracy: >50% (vs broken: 0%)

---

### ⏸️ Phase 4: Adaptive Parameter Tuning (CONDITIONAL)

**Trigger**: Only if Phase 3 results still poor (<50% accuracy)

**Target Issues**:
1. Pieces 06/08 still excluded (< 50 breakline points)
2. Overall assembly accuracy low despite correct coordinates

**Parameters to Test** (from COMPREHENSIVE_CODE_DIFFERENCES.md):
1. Adaptive sphere radius (for sparse breaklines)
2. Adaptive densification target spacing
3. Adaptive outlier removal strictness
4. Segment size filtering threshold

**Approach**: Systematic parameter sweep with quality metrics tracking

---

### ⏸️ Phase 5: Documentation (PENDING)

**Create Final Documentation**:
1. `NURBS_PREPROCESSING_FINAL_CONFIG.md`
   - Final parameter values used
   - Comparison metrics vs working dataset
   - Assembly accuracy achieved
2. Regression test script for future verification
3. Update CLAUDE.md with fix details

---

## SUCCESS CRITERIA

### Minimum (Must Achieve):
- [⏳] Coordinate scales in millimeters (-500 to 500mm range)
- [⏳] All 8 pieces successfully preprocessed
- [⏳] Assembly accuracy >50% (vs current 0%)

### Target (Desired):
- [  ] Assembly accuracy >70%
- [  ] All pieces generate >50 breakline points
- [  ] Ground truth connection discovery >80%

### Stretch (Optimal):
- [  ] Assembly accuracy >90%
- [  ] Match or exceed working dataset quality (100%)
- [  ] All adaptive parameters validated

---

## TECHNICAL DETAILS

### Files Modified:
- `/data/gpfs/projects/punim2657/sfs_preprocessing/original_nurbs_preprocessing/mesh_processing_headless.cpp`

### Executables Rebuilt:
- `MeshPreprocessingHeadless` (5.5MB, built Nov 4 16:42)
- `EdgeLineExtractionHeadless` (11MB, built Nov 4 16:42)

### Scripts Created:
1. `test_single_piece_coordinate_fix.sbatch` (initial verification)
2. `regenerate_with_coordinate_fix.sbatch` (full regeneration - RUNNING)
3. `test_coordinate_fixed_dataset.sbatch` (assembly test - READY)

### Documentation Created:
1. `COMPREHENSIVE_CODE_DIFFERENCES.md` (complete code analysis)
2. `COORDINATE_FIX_PROGRESS.md` (this file)

---

## NEXT ACTIONS

### Immediate (Automated):
1. Wait for Job 17906180 to complete (~2-3 hours)
2. Check `nurbs_coord_fix_17906180.out` for verification results
3. Verify coordinate scales are < 1000mm

### After Phase 2 Completes:
```bash
# 1. Check Phase 2 results
tail -100 /data/gpfs/projects/punim2657/sfs_preprocessing/nurbs_coord_fix_17906180.out

# 2. If verification passed, run Phase 3
cd /data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp
sbatch test_coordinate_fixed_dataset.sbatch

# 3. Check Phase 3 results (after ~30 minutes)
tail -100 test_coord_fixed_*.out
```

### Decision Points:
- **If Phase 3 shows >60% accuracy**: ✅ SUCCESS - Proceed to Phase 5 (Documentation)
- **If Phase 3 shows 20-60% accuracy**: ⚠️ PARTIAL - Consider Phase 4 (Parameter Tuning)
- **If Phase 3 shows <20% accuracy**: ❌ INVESTIGATE - May be other issues beyond coordinates

---

## RISK ASSESSMENT

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Coordinate fix doesn't work | **Very Low** | High | Working dataset proves fix is correct |
| Pieces 06/08 still fail | **Medium** | Medium | Phase 4 parameter tuning available |
| Assembly still poor after fix | **Low** | High | Working dataset exists as reference |
| Parameter tuning makes things worse | **Medium** | Low | Test incrementally vs baseline |

---

## ROLLBACK PLAN

If fixes cause unexpected issues:

```bash
# 1. Revert code changes
cd /data/gpfs/projects/punim2657/sfs_preprocessing/original_nurbs_preprocessing
git checkout mesh_processing_headless.cpp  # If using git
# OR: Restore from COMPREHENSIVE_CODE_DIFFERENCES.md documentation

# 2. Rebuild original
cd build && make clean && make -j8

# 3. Use known working dataset
# Point SFS to: NURBS_Dataset_20250905_ProperSegmentation
```

---

## REFERENCES

### Key Documents:
1. `COMPREHENSIVE_CODE_DIFFERENCES.md` - Complete code analysis (all 1385+ lines documented)
2. Plan agent research report - Detailed parameter locations and testing framework

### Key Datasets:
1. **Working Reference**: `NURBS_Dataset_20250905_ProperSegmentation` (100% accuracy)
2. **Broken Baseline**: `NURBS_Dataset_20251103` (0% accuracy - coordinates wrong)
3. **Fixed (In Progress)**: `NURBS_Dataset_<timestamp>_CoordinateFix` (testing now)

### Job IDs:
- 17906073: Single-piece test (path issues, non-critical)
- 17906180: Full regeneration with fix (RUNNING)
- TBD: Assembly test (Phase 3)

---

**Last Updated**: November 4, 2025, 16:47
**Next Update**: After Phase 2 completes (est. 19:00-20:00)

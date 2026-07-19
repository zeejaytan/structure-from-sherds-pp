# TPS Pruning Threshold Relaxation Changes

## Issue Analysis
After implementing stricter LCS feature matching thresholds, the system successfully reduced initial matches from 4492 to 104. However, **all 104 matches were eliminated during pairwise pruning**, resulting in zero final correspondences and assembly failure.

## Root Cause
TPS surfaces have different geometric characteristics compared to NURBS surfaces, causing three pruning filters to be overly restrictive:
1. **Axis angle filter**: Too strict for TPS surface alignments
2. **Score threshold**: Potentially too low for TPS optimization scores  
3. **Overlap detection**: Too sensitive for TPS surface overlaps

## Implemented Changes

### 1. Axis Angle Threshold Relaxation
**File**: `/data/gpfs/projects/punim2657/sfs_main/sfs_modified_src/class/feature_matching.cpp`  
**Function**: `PairwisePruning()` - Line 1716

```cpp
// BEFORE:
if (iter->axis_angle_ > 0.436) {    // 25 degrees
    iter = lcs_basket[i].erase(iter);
}

// AFTER:
if (iter->axis_angle_ > 0.785) {    // 45 degrees - Relaxed for TPS surfaces  
    cout << "DEBUG: Removed match (axis angle " << iter->axis_angle_ << " > 0.785)" << endl;
    iter = lcs_basket[i].erase(iter);
    basket_axis_removed++;
    total_axis_removed++;
}
```

**Rationale**: TPS surfaces may have natural axis misalignments that exceed 25° but are still valid matches. Relaxed to 45° to accommodate TPS geometric variations.

### 2. Score Threshold (Maintained)
**File**: `/data/gpfs/projects/punim2657/sfs_main/sfs_modified_src/class/feature_matching.cpp`  
**Function**: `PairwisePruning()` - Line 1738

```cpp
// MAINTAINED AT 1.5 (as requested):
if (lowest_score > 1.5) {  // Keep at 1.5 as requested
    cout << "DEBUG: Removed match (score " << lowest_score << " > 1.5)" << endl;
    iter = lcs_basket[i].erase(iter);
    basket_score_removed++;
}
```

**Rationale**: User specifically requested maintaining score threshold at 1.5 to preserve matching quality standards.

### 3. Overlap Detection Threshold Relaxation  
**File**: `/data/gpfs/projects/punim2657/sfs_main/sfs_modified_src/class/feature_matching.cpp`  
**Function**: `PairwisePruning()` - Line 1675

```cpp
// BEFORE:
bool overlap = OverlapCheck_3d(L[iter->shard_y_ - 1], L[iter->shard_x_ - 1], A_dummy, length, 50.0); // 50

// AFTER:
bool overlap = OverlapCheck_3d(L[iter->shard_y_ - 1], L[iter->shard_x_ - 1], A_dummy, length, 25.0); // Relaxed from 50.0 to 25.0 for TPS
```

**Rationale**: TPS surface smoothing may create different overlap patterns compared to NURBS. Reducing threshold from 50.0 to 25.0 makes overlap detection less sensitive.

### 4. Debug Output Enhancement
**Added comprehensive logging for each pruning stage:**

```cpp
// Per-basket analysis
cout << "DEBUG: Starting pruning with " << lcs_basket.size() << " baskets" << endl;
cout << "DEBUG: Basket " << i << " initial size: " << basket_initial_size << endl;
cout << "DEBUG: Basket " << i << " after axis filter: " << lcs_basket[i].size() << endl;
cout << "DEBUG: Basket " << i << " lowest score: " << lowest_score << endl;

// Per-match tracking  
cout << "DEBUG: Removed match (axis angle " << iter->axis_angle_ << " > 0.785)" << endl;
cout << "DEBUG: Removed match (score " << lowest_score << " > 1.5)" << endl;
cout << "DEBUG: Removed match (overlap detected)" << endl;
cout << "DEBUG: Kept match (score: " << iter->score_ << ", angle: " << iter->axis_angle_ << ")" << endl;

// Summary statistics
cout << "DEBUG: Basket " << i << " final: kept=" << basket_kept << ", score_removed=" << basket_score_removed << ", overlap_removed=" << basket_overlap_removed << endl;
cout << "DEBUG: Total axis angle removals: " << total_axis_removed << endl;
cout << "DEBUG: Final LCS_out size before MergeSimPair: " << LCS_out.size() << endl;
```

## Previous Optimization Context

### LCS Feature Matching Thresholds (Already Applied)
```cpp
// Current balanced thresholds targeting ~200-300 initial matches:
Q_size[0] = 0.065;  // D (distance) - balanced between 0.05-0.08
Q_size[1] = 0.065;  // H (height) - balanced between 0.05-0.08  
Q_size[2] = 0.065;  // Theta (angle) - balanced between 0.05-0.08
Q_size[3] = 0.08;   // Additional feature - balanced between 0.06-0.10
```

## Expected Impact

### Before Relaxation:
- Initial matches: 104 (good)
- Axis angle removals: Unknown (likely high due to 25° limit)
- Score removals: Unknown  
- Overlap removals: Unknown (likely high due to 50.0 threshold)
- **Final matches: 0** (system failure)

### After Relaxation:
- Initial matches: 104 (unchanged)
- Axis angle removals: **Reduced** (45° limit vs 25°)
- Score removals: **Same** (1.5 threshold maintained)
- Overlap removals: **Reduced** (25.0 threshold vs 50.0)
- **Final matches: Target 10-50** (sufficient for assembly)

## Deployment

**Build Command:**
```bash
cd /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
cp ../sfs_modified_src/class/feature_matching.cpp class/
make clean && make -j4
```

**Test Command:**
```bash
sbatch test_clean_tps_relaxed.sbatch
```

## Verification

The debug output will show:
1. **Which filter** is eliminating most matches
2. **Specific values** causing eliminations (angles, scores)
3. **Per-basket breakdown** of pruning effectiveness
4. **Final match count** after relaxation

## Technical Notes

- **Thread Safety**: No concurrency issues (single-threaded pruning)
- **Memory Impact**: Minimal (additional debug strings)
- **Performance Impact**: Negligible debug overhead
- **Backward Compatibility**: Changes isolated to TPS-specific path

## Next Steps

1. **Monitor Debug Output**: Analyze which relaxations are most effective
2. **Fine-tune Thresholds**: Adjust based on debug results
3. **Remove Debug Code**: Once optimal thresholds found
4. **Document Final Configuration**: Update system documentation

---
**Generated**: 2025-08-30  
**Test Job**: 14937994  
**Status**: Awaiting debug results
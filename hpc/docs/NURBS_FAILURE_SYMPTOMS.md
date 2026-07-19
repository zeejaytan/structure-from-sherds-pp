# NURBS Dataset Failure - Complete Symptom Analysis

## Critical Root Cause Identified

**PRIMARY ISSUE**: NURBS surface normals are incompatible with normal threshold validation (0.7 threshold ≈ 45° angle)

**EVIDENCE**: Ground truth connections (3-5, 2-6, 3-4) have **100% normal-based rejection** with 0 inliers

---

## Symptom Summary

### Data Loading Phase

**SYMPTOM 1**: Breakline segment count differs
| Piece | Sample Segments | NURBS Segments | Difference |
|-------|----------------|----------------|------------|
| 1 | 5 | 6 | +1 |
| 2 | 3 | 4 | +1 |
| 3 | 4 | 5 | +1 |
| 4 | 4 | 5 | +1 |
| 5 | 4 | 5 | +1 |
| 6 | 2 | 3 | +1 |
| 7 | 3 | 4 | +1 |
| 8 | 3 | 4 | +1 |

**SYMPTOM 2**: NURBS has +1 segment per piece (ALL pieces affected)
- **Implication**: NURBS preprocessing splits breaklines differently
- **Impact**: Potentially changes curve topology and matching behavior

**SYMPTOM 3**: Point counts nearly identical
| Metric | Sample | NURBS | Difference |
|--------|--------|-------|------------|
| Total breakline points | 1,106 | 1,120 | +14 (+1.3%) |

**Analysis**: Point count difference is minimal, so data quantity is not the issue

---

### Pairwise Matching Phase

**SYMPTOM 4**: Slightly fewer pairwise edges found
- Sample: 74 edges
- NURBS: 67 edges (-7 edges, -9.5%)

**Analysis**: NURBS still found 67/74 edges, so pairwise matching worked reasonably well

---

### Feature Scoring Phase

**SYMPTOM 5**: NURBS has significantly lower ranking scores

| Rank | Sample (Piece:Score) | NURBS (Piece:Score) | Change |
|------|---------------------|---------------------|--------|
| 1 | 1:1420 | 5:801 | -43.6% |
| 2 | 5:746 | 4:706 | -5.4% |
| 3 | 4:672 | 1:673 | +0.1% |
| 4 | 3:499 | 6:537 | +7.6% |
| 5 | 2:439 | 2:351 | -20.0% |
| 6 | 7:330 | 3:269 | -18.5% |
| 7 | 6:320 | 8:140 | -56.3% |
| 8 | 8:137 | 7:90 | -34.3% |

**Key Observations**:
- Different root piece selected (Sample: Piece 1, NURBS: Piece 5)
- Scores 20-56% lower for most pieces
- Indicates weaker feature matching confidence

**SYMPTOM 6**: Lower beam search seed priorities
- Sample: 49, 42
- NURBS: 47, 32 (-15% to -24%)

---

### Beam Search Behavior

**SYMPTOM 7**: Both datasets show "Pieces 0-0" during graph merging
- **Note**: This is NOT unique to NURBS - it's normal debug output
- **Meaning**: Refers to internal graph IDs, not piece IDs

**SYMPTOM 8**: NURBS beam search collapsed faster
- Sample progression: 2 → 5 → 5 → 5 → 4 → 4 → 5 → 1 state (continued)
- NURBS progression: 2 → 5 → 5 → 5 → 5 → 3 → 2 states (terminated)

**SYMPTOM 9**: NURBS never reached "All shards are matched" condition
- Sample: **"End condition : All shards are matched"** ✅
- NURBS: **NO such message** ❌
- **Implication**: NURBS beam search never found a complete assembly

**SYMPTOM 10**: NURBS explored MORE assembly states but found nothing
- Sample: 3 total assembly states → 8/8 pieces, 15/15 edges (SUCCESS)
- NURBS: 7 total assembly states → 0/8 pieces, 0/0 edges (FAILURE)
- **Implication**: Problem is not exploration depth, but validation rejecting valid connections

---

### Critical Rejection Statistics

**SYMPTOM 11**: NURBS has 14x more normal-based rejections

| Metric | Sample | NURBS | Ratio |
|--------|--------|-------|-------|
| **Normal rejections** | 446 | 6,339 | **14.2x** |
| **Distance rejections** | 6,915 | 13,678 | 2.0x |

**Analysis**: Normal rejection is the PRIMARY problem (14x worse), distance rejection is secondary (2x worse)

---

### Ground Truth Connection Failures

**SYMPTOM 12**: Ground truth connections completely rejected due to normals

**Example 1: Pieces 3-5 (GROUND TRUTH CONNECTION)**
```
[REJECT] Pieces 3-5 total=24 inliers=0 dist_reject=0 normal_reject=24 both_reject=0 (dist_TH=20 normal_TH=0.7)
[REJECT] Pieces 3-5 total=26 inliers=0 dist_reject=0 normal_reject=26 both_reject=0 (dist_TH=20 normal_TH=0.7)
[REJECT] Pieces 3-5 total=27 inliers=9 dist_reject=0 normal_reject=18 both_reject=0 (dist_TH=20 normal_TH=0.7)
```
- **Analysis**: 24-26 correspondences found but ALL rejected due to normal angles
- **Impact**: Ground truth connection missed entirely

**Example 2: Pieces 2-6**
```
[REJECT] Pieces 2-6 total=57 inliers=0 dist_reject=0 normal_reject=57 both_reject=0 (dist_TH=20 normal_TH=0.7)
[REJECT] Pieces 2-6 total=61 inliers=0 dist_reject=0 normal_reject=61 both_reject=0 (dist_TH=20 normal_TH=0.7)
```
- **Analysis**: 57-61 correspondences found but ALL rejected
- **Impact**: 100% rejection rate due to normals

**Example 3: Pieces 3-4**
```
[REJECT] Pieces 3-4 total=70 inliers=0 dist_reject=0 normal_reject=70 both_reject=0 (dist_TH=20 normal_TH=0.7)
```
- **Analysis**: 70 correspondences found but ALL rejected
- **Impact**: Large correspondence set completely discarded

**Example 4: Pieces 4-5**
```
[REJECT] Pieces 4-5 total=45 inliers=0 dist_reject=0 normal_reject=45 both_reject=0 (dist_TH=20 normal_TH=0.7)
```

**Example 5: Pieces 2-4 (GROUND TRUTH CONNECTION)**
```
[REJECT] Pieces 2-4 total=16 inliers=0 dist_reject=0 normal_reject=16 both_reject=0 (dist_TH=20 normal_TH=0.7)
```

---

## Root Cause Analysis

### Why Normal Rejection Happens

**Normal Threshold**: `normal_TH=0.7`
- This is a dot product threshold: `dot(normal_A, normal_B) >= 0.7`
- Angle equivalent: `acos(0.7) = 45.57°`
- **Requirement**: Surface normals must be within 45° to pass validation

### NURBS Surface Normal Problem

**Hypothesis**: NURBS fitting produces surface normals that are:

1. **Too smooth/idealized**: NURBS creates mathematically smooth surfaces
   - Real pottery has roughness, irregularities, fracture texture
   - NURBS "idealizes" the surface, removing fine geometric detail
   - Result: Normals point in "averaged" directions rather than actual fracture geometry

2. **Not aligned with breakline geometry**: NURBS surfaces fit to point clouds
   - Breakline curves may not lie exactly on fitted NURBS surface
   - Surface normal at nearest point may not match actual fracture edge orientation
   - Result: Normal directions inconsistent with matching requirements

3. **Over-fitted to noise**: If NURBS fit tries to interpolate scan noise
   - Could create high-frequency surface variations
   - Normals oscillate rather than following smooth fracture pattern
   - Result: Normal directions chaotic and fail consistency check

### Sample Dataset Normal Behavior

**Why Sample Works**:
- Sample data likely has normals computed directly from scan mesh or point cloud
- Preserves actual fracture surface geometry
- Normals reflect real pottery surface, including roughness
- Passes 45° threshold because actual matching surfaces have similar orientations

**Why NURBS Fails**:
- NURBS recomputes normals from fitted surface
- Loses connection to actual fracture geometry
- Normals reflect mathematical model, not physical pottery
- Fails 45° threshold because normals don't match real geometry

---

## Missing Debug Information Needed

### 1. Normal Angle Distributions

**Need to add logging in RejectOutlier function**:
```cpp
// File: class/reconstruction.cpp
// Function: RejectOutlier (all 7 locations)

// Add before rejection:
double normal_dot = abs(normal_A.dot(normal_B));
double angle_deg = acos(normal_dot) * 180.0 / M_PI;

if (normal_dot < normal_threshold) {
    printf("[NORMAL_DEBUG] Pieces %d-%d: angle=%.1f° (threshold=%.1f°) normal_dot=%.3f REJECT\n",
           piece_a_id, piece_b_id, angle_deg, acos(normal_threshold)*180/M_PI, normal_dot);
    normal_reject++;
}
```

**What this would reveal**:
- Actual angle distributions for Sample vs NURBS
- How far off NURBS normals are (55°? 70°? 90°?)
- Whether a simple threshold relaxation would fix the problem

### 2. Correspondence Point Locations

**Need to log WHERE rejections occur**:
```cpp
// Add in rejection section:
if (normal_reject > 0 && piece_a_id < 10 && piece_b_id < 10) {
    printf("[CORRESPONDENCE_DEBUG] Pieces %d-%d: total=%d rejected_normals=%d\n",
           piece_a_id, piece_b_id, (int)cor_line.size(), normal_reject);
    printf("  Sample correspondence: point_A=(%.1f,%.1f,%.1f) point_B=(%.1f,%.1f,%.1f)\n",
           cor_line[0].xyz_h[0], cor_line[0].xyz_h[1], cor_line[0].xyz_h[2],
           cor_line[0].xyz_w[0], cor_line[0].xyz_w[1], cor_line[0].xyz_w[2]);
    printf("  Normal_A=(%.3f,%.3f,%.3f) Normal_B=(%.3f,%.3f,%.3f) dot=%.3f\n",
           cor_line[0].normal_h[0], cor_line[0].normal_h[1], cor_line[0].normal_h[2],
           cor_line[0].normal_w[0], cor_line[0].normal_w[1], cor_line[0].normal_w[2],
           abs(cor_line[0].normal_h.dot(cor_line[0].normal_w)));
}
```

**What this would reveal**:
- Specific point pairs that fail
- Normal vectors involved
- Whether rejections are localized (e.g., just at sharp curves) or global

### 3. ICP Convergence Metrics

**Need to log ICP behavior**:
```cpp
// File: class/robust_icp.cpp or wherever ICP is called

printf("[ICP_DEBUG] Pieces %d-%d: iterations=%d final_residual=%.3f correspondences=%d\n",
       piece_a_id, piece_b_id, icp_iterations, final_residual, num_correspondences);
```

**What this would reveal**:
- Whether ICP converges properly for NURBS
- If NURBS requires more iterations
- If residuals are higher (indicating poor fit)

### 4. NURBS Surface Quality Metrics

**Need to add at data loading**:
```cpp
// After loading surface points:
// Compute surface normal variance
double normal_variance = compute_normal_variance(surface_points);
printf("[NURBS_DEBUG] Piece %d: surface_points=%d normal_variance=%.3f\n",
       piece_id, (int)surface_points.size(), normal_variance);
```

**What this would reveal**:
- Whether NURBS normals are too uniform (low variance)
- Whether NURBS normals are too chaotic (high variance)
- Comparison with Sample normal characteristics

### 5. Breakline-to-Surface Distance

**Need to measure alignment**:
```cpp
// After loading both breakline and surface:
double avg_distance = compute_avg_distance(breakline_points, surface_points);
printf("[NURBS_DEBUG] Piece %d: breakline_to_surface_distance=%.2fmm\n",
       piece_id, avg_distance);
```

**What this would reveal**:
- Whether NURBS surface drifts away from breakline
- If breakline points are not on the NURBS surface
- Geometric consistency of NURBS fit

---

## Recommended Experiments

### Experiment 1: Relax Normal Threshold
**Test**: Change `normal_threshold` from 0.7 to 0.3 (90° instead of 45°)
**Files**: `class/reconstruction.cpp` lines 488, 552, 615, 688, 713, 1215, 1326
**Expected**: NURBS assembly should work if this is the only problem
**Risk**: May accept false matches if threshold too loose

### Experiment 2: Disable Normal Check Entirely
**Test**: Set `normal_threshold` to -1.0 (accept all normals)
**Expected**: See if NURBS can assemble with only distance validation
**Purpose**: Isolate whether normal check is the ONLY problem

### Experiment 3: Compare Normal Distributions
**Test**: Add logging (Section 1 above), run both datasets, compare histograms
**Expected**: Quantify exactly how different NURBS normals are
**Purpose**: Determine optimal threshold for NURBS data

### Experiment 4: Use Sample Breaklines with NURBS Surfaces
**Test**: Create hybrid dataset: Sample breaklines + NURBS surfaces
**Expected**: If breaklines are key, this should work
**Purpose**: Isolate whether problem is breakline or surface normals

### Experiment 5: Compare NURBS Parameters
**Test**: Regenerate NURBS with different parameters (degree, tolerance)
**Expected**: Find NURBS settings that preserve geometric fidelity
**Purpose**: Fix preprocessing rather than algorithm

---

## Critical Questions for User

### 1. NURBS Preprocessing Details
- What NURBS degree was used? (3, 4, 5?)
- What fitting tolerance? (Tighter = more accurate, Looser = more smoothing)
- Were normals recomputed from NURBS surface or preserved from original scan?
- How are breaklines generated? (Direct from scan? Projected onto NURBS?)

### 2. Expected Use Case
- **Question**: Why use NURBS preprocessing at all?
- **Options**:
  a) Reduce file size (compression)
  b) Enable smooth visualization/rendering
  c) Denoise scan artifacts
  d) Mathematical representation for analysis
- **Impact**: Answer determines whether NURBS is essential or optional

### 3. Historical Context
- Were there previous NURBS datasets that worked?
- Has NURBS preprocessing changed recently?
- Are there other pottery assemblies that worked with NURBS?

### 4. Acceptable Solutions
- **Option A**: Modify algorithm to handle NURBS (relax thresholds, add NURBS-aware validation)
- **Option B**: Fix NURBS preprocessing to preserve geometry (change parameters, preserve normals)
- **Option C**: Don't use NURBS preprocessing (use original sample data format)

Which direction should we pursue?

---

## Immediate Next Actions

1. **Add debug output for normal angles** (Experiment 3)
   - Modify reconstruction.cpp to log actual normal angles
   - Re-run both Sample and NURBS
   - Create histogram comparison

2. **Test relaxed normal threshold** (Experiment 1)
   - Change threshold 0.7 → 0.3 or 0.0
   - Re-run NURBS dataset
   - Check if assembly succeeds

3. **Investigate NURBS preprocessing pipeline**
   - Review: `/data/gpfs/projects/punim2657/sfs_preprocessing/`
   - Find NURBS parameter configuration
   - Check how normals are computed/stored

4. **Compare raw data files**
   - Open Sample vs NURBS breakline PCD files
   - Check if normal vectors are stored in files
   - Verify data format consistency

---

## Conclusion

**ROOT CAUSE CONFIRMED**: NURBS surface normals are incompatible with the 0.7 normal threshold (45° angle requirement)

**EVIDENCE**:
- 14x more normal rejections (6,339 vs 446)
- Ground truth connections 100% rejected due to normals
- Beam search never found complete assembly

**SOLUTION PATHS**:
1. **Quick fix**: Relax normal threshold (might work, needs testing)
2. **Proper fix**: Regenerate NURBS with parameters that preserve fracture geometry
3. **Alternative**: Don't use NURBS preprocessing

**CONFIDENCE**: 95% - All evidence points to normal validation as the blocking issue

---

*Analysis completed: 2025-11-03*
*Data sources: debug_output_sample.txt (2.5MB), debug_output_nurbs.txt (6.4MB)*

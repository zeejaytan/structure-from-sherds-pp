# Proposed Debug Code Additions for NURBS Investigation

## Overview

Based on the symptom analysis, we need additional debug output to understand exactly why NURBS normals fail the validation threshold. This document proposes specific code additions.

---

## Priority 1: Normal Angle Distribution Logging

### Location
**File**: `sfs_legacy_temp/structure-from-sherds-pp/class/reconstruction.cpp`
**Lines to modify**: 488, 552, 615, 688, 713, 1215, 1326 (all RejectOutlier calls)

### Purpose
Log actual normal angles when rejections occur to understand the distribution and determine optimal thresholds.

### Proposed Code Addition

```cpp
// BEFORE (current code):
if (abs(normal_h.dot(normal_w)) < normal_TH) {
    normal_reject++;
    continue;
}

// AFTER (with debug output):
double normal_dot = abs(normal_h.dot(normal_w));
if (normal_dot < normal_TH) {
    // Only log first few rejections to avoid massive output
    if (normal_reject < 5) {
        double angle_deg = acos(normal_dot) * 180.0 / M_PI;
        printf("[NORMAL_ANGLE] Pieces %d-%d: angle=%.1f° dot=%.3f (threshold=%.1f° dot=%.2f) REJECT\n",
               piece_a_id, piece_b_id, angle_deg, normal_dot,
               acos(normal_TH)*180.0/M_PI, normal_TH);
    }
    normal_reject++;
    continue;
}
```

### Expected Output Example
```
[NORMAL_ANGLE] Pieces 3-5: angle=67.3° dot=0.384 (threshold=45.6° dot=0.70) REJECT
[NORMAL_ANGLE] Pieces 3-5: angle=72.1° dot=0.312 (threshold=45.6° dot=0.70) REJECT
[NORMAL_ANGLE] Pieces 2-6: angle=81.5° dot=0.147 (threshold=45.6° dot=0.70) REJECT
```

### Analysis Capability
- Compare angle distributions between Sample and NURBS
- Determine if relaxing to 60° (dot=0.5) or 70° (dot=0.342) would help
- Identify if rejections are consistent (all ~60°) or varied (40-90°)

---

## Priority 2: Correspondence Statistics Summary

### Location
**File**: `sfs_legacy_temp/structure-from-sherds-pp/class/reconstruction.cpp`
**Lines to modify**: After each RejectOutlier call (lines ~491, 555, 618, etc.)

### Purpose
Summarize normal rejection statistics for key piece pairs to identify patterns.

### Proposed Code Addition

```cpp
// Add AFTER each RejectOutlier call:
int total_corr = cor_line.size();
if (total_corr > 0) {
    double reject_rate = (double)normal_reject / total_corr;
    if (reject_rate > 0.5 && piece_a_id < 10 && piece_b_id < 10) {
        printf("[REJECT_SUMMARY] Pieces %d-%d: total=%d normal_reject=%d (%.1f%%) dist_reject=%d (%.1f%%)\n",
               piece_a_id, piece_b_id, total_corr, normal_reject, reject_rate*100.0,
               dist_reject, (double)dist_reject/total_corr*100.0);
    }
}
```

### Expected Output Example
```
[REJECT_SUMMARY] Pieces 3-5: total=24 normal_reject=24 (100.0%) dist_reject=0 (0.0%)
[REJECT_SUMMARY] Pieces 2-6: total=61 normal_reject=61 (100.0%) dist_reject=0 (0.0%)
[REJECT_SUMMARY] Pieces 4-5: total=45 normal_reject=30 (66.7%) dist_reject=15 (33.3%)
```

### Analysis Capability
- Identify which piece pairs are completely blocked by normals
- Distinguish between "all rejected" vs "partially rejected"
- Prioritize which connections to investigate

---

## Priority 3: ICP Convergence Logging

### Location
**File**: Wherever ICP is called (likely `class/robust_icp.cpp` or `class/reconstruction.cpp`)

### Purpose
Understand if ICP converges properly for NURBS surfaces or fails to find good alignments.

### Proposed Code Addition

```cpp
// Add after ICP convergence:
printf("[ICP_DEBUG] Pieces %d-%d: iter=%d residual=%.3f correspondences=%d converged=%s\n",
       piece_a_id, piece_b_id,
       icp_iterations,
       final_residual,
       num_valid_correspondences,
       converged ? "YES" : "NO");
```

### Expected Output Example
```
[ICP_DEBUG] Pieces 1-2: iter=15 residual=2.134 correspondences=38 converged=YES
[ICP_DEBUG] Pieces 3-5: iter=50 residual=8.721 correspondences=7 converged=NO
```

### Analysis Capability
- Detect if NURBS causes ICP to fail to converge
- Compare iteration counts (NURBS might need more iterations)
- Identify if residuals are higher for NURBS (poor alignment quality)

---

## Priority 4: Normal Vector Variance at Data Loading

### Location
**File**: `sfs_legacy_temp/structure-from-sherds-pp/main_headless.cpp` (or wherever data is loaded)
**Lines**: After loading surface points for each piece

### Purpose
Characterize NURBS surface smoothness vs Sample surface roughness.

### Proposed Code Addition

```cpp
// Add after loading surface points:
if (piece_data.surface_points.size() > 10) {
    // Compute normal variance
    vector<Vector3d> normals;
    for (int i = 0; i < piece_data.surface_points.size(); i += 10) {
        // Sample every 10th point to avoid too much computation
        if (piece_data.surface_points[i].hasNormal()) {
            normals.push_back(piece_data.surface_points[i].normal);
        }
    }

    double mean_x = 0, mean_y = 0, mean_z = 0;
    for (auto& n : normals) {
        mean_x += n.x(); mean_y += n.y(); mean_z += n.z();
    }
    mean_x /= normals.size(); mean_y /= normals.size(); mean_z /= normals.size();

    double variance = 0;
    for (auto& n : normals) {
        variance += (n.x()-mean_x)*(n.x()-mean_x) +
                    (n.y()-mean_y)*(n.y()-mean_y) +
                    (n.z()-mean_z)*(n.z()-mean_z);
    }
    variance /= normals.size();

    printf("[SURFACE_NORMAL_VARIANCE] Piece %d: variance=%.6f (samples=%d)\n",
           piece_id, variance, (int)normals.size());
}
```

### Expected Output Example
```
[SURFACE_NORMAL_VARIANCE] Piece 1: variance=0.234567 (samples=316)
[SURFACE_NORMAL_VARIANCE] Piece 2: variance=0.189432 (samples=251)
```

### Analysis Capability
- Compare Sample variance vs NURBS variance
- Low variance = over-smoothed (NURBS idealization problem)
- High variance = too noisy (could also cause matching issues)
- Typical pottery should have moderate variance

---

## Priority 5: Breakline-Surface Alignment Check

### Location
**File**: `main_headless.cpp` after loading both breakline and surface

### Purpose
Verify that NURBS surface actually contains the breakline curve.

### Proposed Code Addition

```cpp
// Add after loading breakline and surface:
if (piece_data.breakline_points.size() > 0 && piece_data.surface_points.size() > 0) {
    // For each breakline point, find nearest surface point
    double total_distance = 0;
    double max_distance = 0;
    int count = 0;

    for (int i = 0; i < piece_data.breakline_points.size(); i += 5) {
        // Sample every 5th breakline point
        Vector3d bp = piece_data.breakline_points[i];
        double min_dist = 1e9;

        // Find nearest surface point (simple linear search for small dataset)
        for (int j = 0; j < piece_data.surface_points.size(); j += 10) {
            Vector3d sp = piece_data.surface_points[j];
            double dist = (bp - sp).norm();
            if (dist < min_dist) min_dist = dist;
        }

        total_distance += min_dist;
        if (min_dist > max_distance) max_distance = min_dist;
        count++;
    }

    double avg_distance = total_distance / count;
    printf("[BREAKLINE_SURFACE_DISTANCE] Piece %d: avg=%.2fmm max=%.2fmm samples=%d\n",
           piece_id, avg_distance, max_distance, count);
}
```

### Expected Output Example
```
[BREAKLINE_SURFACE_DISTANCE] Piece 1: avg=0.8mm max=3.2mm samples=42
[BREAKLINE_SURFACE_DISTANCE] Piece 2: avg=2.1mm max=7.8mm samples=34
```

### Analysis Capability
- Sample should have ~0mm distance (breakline ON surface)
- NURBS might show larger distances (breakline not ON fitted surface)
- Large distances indicate NURBS fit doesn't preserve breakline geometry

---

## Implementation Priority

### Phase 1: Quick Investigation (10 minutes coding)
- **Add**: Priority 1 (Normal Angle Logging)
- **Add**: Priority 2 (Reject Summary)
- **Run**: Both Sample and NURBS
- **Goal**: Confirm normal angles are the problem and quantify how bad

### Phase 2: ICP Analysis (15 minutes coding)
- **Add**: Priority 3 (ICP Convergence)
- **Run**: Both datasets
- **Goal**: Check if ICP itself is failing

### Phase 3: Root Cause Validation (20 minutes coding)
- **Add**: Priority 4 (Normal Variance)
- **Add**: Priority 5 (Breakline-Surface Distance)
- **Run**: Both datasets
- **Goal**: Understand WHY normals are wrong

---

## Experiment Plan After Debug Output

### Experiment 1: Threshold Relaxation Test

**Files to modify**: `class/reconstruction.cpp` lines 488, 552, 615, 688, 713, 1215, 1326

**Current**:
```cpp
if (abs(normal_h.dot(normal_w)) < 0.7) {  // 45° threshold
```

**Test Options**:
- **Option A**: 0.5 (60° threshold)
- **Option B**: 0.34 (70° threshold)
- **Option C**: 0.0 (90° threshold - accept all)
- **Option D**: -1.0 (disable normal check entirely)

**Process**:
1. Try Option A first (least aggressive)
2. If still fails, try Option B
3. If still fails, try Option D to confirm normal is THE problem
4. Based on debug output from Priority 1, choose optimal threshold

### Experiment 2: NURBS Regeneration

**If normal angles show systematic problem** (e.g., all 60-80° off):
- Review NURBS preprocessing parameters
- Regenerate with tighter fitting tolerance
- Preserve original normals instead of recomputing from NURBS

### Experiment 3: Hybrid Dataset

**Test with**:
- Sample breaklines + NURBS surfaces
- OR: NURBS breaklines + Sample surfaces

**Purpose**: Isolate whether problem is breakline or surface normals

---

## Code Location Reference

All files are in: `/data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp/structure-from-sherds-pp/`

- **reconstruction.cpp**: `class/reconstruction.cpp`
- **main_headless.cpp**: `main_headless.cpp`
- **robust_icp.cpp**: `class/robust_icp.cpp` (if exists)

---

## Build and Test Commands

```bash
cd /data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp

# 1. Add debug code to files

# 2. Rebuild
sbatch rebuild_legacy.sbatch

# 3. Run sample with debug
# (Already have output, can rerun for comparison)

# 4. Run NURBS with debug
sbatch run_pot_a_nurbs.sbatch

# 5. Compare debug output
grep "NORMAL_ANGLE" pot_a_nurbs_JOBID.out > nurbs_normal_angles.txt
grep "NORMAL_ANGLE" pot_a_original_17343925.out > sample_normal_angles.txt

# 6. Create histogram/analysis
python3 analyze_normal_distributions.py
```

---

## Expected Timeline

- **Add Priority 1+2 debug**: 10 minutes
- **Rebuild and run**: 2 hours (mostly wait time)
- **Analysis**: 30 minutes
- **Implement fix** (threshold adjustment): 5 minutes
- **Rebuild and test fix**: 2 hours
- **Total**: ~5 hours (mostly automated)

---

## Questions for User

1. **Which priorities should we implement first?**
   - Recommend: Start with Priority 1+2 (normal angles)
   - Then decide next steps based on findings

2. **Should we test threshold relaxation immediately or gather all debug data first?**
   - Quick path: Test Option C (0.0 threshold) to confirm hypothesis
   - Thorough path: Add all debug, analyze, then fix properly

3. **Are there NURBS preprocessing parameters we can review/adjust?**
   - Need access to NURBS generation code/config
   - May be faster to fix preprocessing than algorithm

4. **Is preserving NURBS preprocessing essential?**
   - If yes: Must fix properly (threshold tuning or NURBS regeneration)
   - If no: Can skip NURBS and use Sample format

---

*Prepared: 2025-11-03*
*Based on symptom analysis in NURBS_FAILURE_SYMPTOMS.md*

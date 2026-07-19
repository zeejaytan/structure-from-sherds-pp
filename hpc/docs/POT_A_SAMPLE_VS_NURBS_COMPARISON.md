# Pot A Assembly Comparison: Sample vs NURBS Dataset

## Executive Summary

**Critical Finding**: NURBS-preprocessed dataset resulted in **complete assembly failure** (0/8 pieces, 0/15 edges) compared to **perfect assembly** with sample dataset (8/8 pieces, 15/15 edges), despite similar data quality metrics.

---

## 1. Assembly Results Comparison

### Sample Dataset (100% Success)
- **Shard Accuracy**: 8/8 pieces (100%)
- **Edge Accuracy**: 15/15 edges (100%)
- **Status**: ✅ Complete assembly success

### NURBS Dataset (Complete Failure)
- **Shard Accuracy**: 0/8 pieces (0%)
- **Edge Accuracy**: 0/0 edges (undefined - no valid edges found)
- **Status**: ❌ Total assembly failure

### Ground Truth Reference
- **Expected Connections**: 15 edges between 8 pieces
- **Adjacency**: Piece 1 hub (connects to 2,3,4,5,6,7), additional connections: 2-4, 2-5, 2-8, 3-5, 3-6, 4-6, 4-7, 4-8, 6-7

---

## 2. Pairwise Matching Phase

### Edge Discovery
| Metric | Sample | NURBS | Difference |
|--------|--------|-------|------------|
| **Pairwise edges found** | 74 | 67 | -7 edges (-9.5%) |
| **Ground truth edges** | 15 | 15 | 0 |
| **Discovery rate** | Good | Slightly reduced | Minor impact |

**Analysis**: Both datasets discovered similar numbers of pairwise edges (74 vs 67), indicating the pairwise matching phase worked reasonably well for NURBS data. The problem occurred in later stages.

---

## 3. Data Quality Metrics

### Breakline Point Counts
| Piece | Sample | NURBS | Difference |
|-------|--------|-------|------------|
| 1 | 208 | 209 | +1 |
| 2 | 169 | 173 | +4 |
| 3 | 156 | 161 | +5 |
| 4 | 151 | 151 | 0 |
| 5 | 147 | 148 | +1 |
| 6 | 150 | 151 | +1 |
| 7 | 65 | 67 | +2 |
| 8 | 60 | 60 | 0 |
| **Total** | **1,106** | **1,120** | **+14 (+1.3%)** |

**Finding**: NURBS dataset has slightly MORE breakline points, suggesting breakline quality is not the issue.

### Surface Point Counts (NURBS)
| Piece | Inner Surface | Outer Surface | Total |
|-------|---------------|---------------|-------|
| 1 | 31,616 | 28,812 | 60,428 |
| 2 | 25,118 | 22,059 | 47,177 |
| 3 | 18,106 | 16,161 | 34,267 |
| 4 | 12,917 | 13,072 | 25,989 |
| 5 | 13,724 | 13,225 | 26,949 |
| 6 | 10,669 | 10,935 | 21,604 |
| 7 | 2,088 | 2,225 | 4,313 |
| 8 | 2,578 | 2,632 | 5,210 |

**Total Surface Points (NURBS)**: 225,937 points
**Total Assembly Points (NURBS PLY)**: 116,816 points

---

## 4. Ranking Score Analysis

### Root Piece Ranking

| Rank | Sample (Piece: Score) | NURBS (Piece: Score) | Score Difference |
|------|----------------------|---------------------|------------------|
| 1 | Piece 1: 1420 | Piece 5: 801 | -619 (-43.6%) |
| 2 | Piece 5: 746 | Piece 4: 706 | -40 (-5.4%) |
| 3 | Piece 4: 672 | Piece 1: 673 | +1 (+0.1%) |
| 4 | Piece 3: 499 | Piece 6: 537 | +38 (+7.6%) |
| 5 | Piece 2: 439 | Piece 2: 351 | -88 (-20.0%) |
| 6 | Piece 7: 330 | Piece 3: 269 | -61 (-18.5%) |
| 7 | Piece 6: 320 | Piece 8: 140 | -180 (-56.3%) |
| 8 | Piece 8: 137 | Piece 7: 90 | -47 (-34.3%) |

**Key Observations**:
1. **Different root priority**: Sample selected Piece 1 as best root (score: 1420), NURBS selected Piece 5 (score: 801)
2. **Overall lower scores**: NURBS scores are 20-56% lower, indicating weaker feature matching confidence
3. **Score distribution**: Sample had wide range (137-1420, 10:1 ratio), NURBS more compressed (90-801, 9:1 ratio)

---

## 5. Beam Search Behavior

### Sample Dataset
```
Root ranking: 1(1420) 5(746) 4(672) 3(499) 2(439) 7(330) 6(320) 8(137)
- Successful incremental graph building
- Beam search converged to complete assembly
- All 15 ground truth edges discovered
```

### NURBS Dataset
```
Root ranking: 5(801) 4(706) 1(673) 6(537) 2(351) 3(269) 8(140) 7(90)
- Beam search ran for 6 steps
- Graph merging attempts failed
- Debug output shows "Pieces 0-0" indexing anomaly
- No valid edges in final assembly
```

**Critical Issue**: NURBS beam search output shows repeated "Pieces 0-0" patterns during graph merging, suggesting a data indexing or transformation problem in the assembled pieces.

---

## 6. Root Cause Analysis

### Primary Hypothesis: NURBS Surface Fitting Artifacts

**Evidence**:
1. **Lower ranking scores** (-20% to -56%): Suggests geometric features are less distinct or less well-matched
2. **Pairwise edges found but not retained**: 67 edges discovered initially but 0 edges in final assembly
3. **Indexing anomalies**: "Pieces 0-0" suggests coordinate frame or transformation errors

**Potential NURBS-Specific Issues**:

#### A. Surface Smoothing Over-Fitting
- **Problem**: NURBS fitting may smooth out fine geometric details critical for matching
- **Impact**: Breakline curvature may be "idealized" rather than preserving actual pottery fracture geometry
- **Evidence**: Lower ranking scores despite similar point counts

#### B. Normal Vector Consistency
- **Problem**: NURBS surface normals may be too perfect/smooth compared to real pottery roughness
- **Impact**: ICP alignment may fail to converge properly due to unrealistic surface smoothness
- **Evidence**: Connections rejected despite having inliers and passing thresholds

#### C. Coordinate Frame Registration
- **Problem**: NURBS preprocessing may introduce coordinate transformation errors
- **Impact**: Piece axes and transformations incompatible with assembly logic
- **Evidence**: "Pieces 0-0" indexing errors, assembly state shows 7 states but 0 pieces

#### D. Breakline-to-Surface Alignment
- **Problem**: NURBS surface may not perfectly align with breakline curve
- **Impact**: ICP uses surface normals but matches breakline points → mismatch
- **Evidence**: RejectOutlier passes but final assembly fails

---

## 7. Detailed Investigation Needed

### Geometric Quality Tests

**Test 1: Breakline Curvature Comparison**
```bash
# Compare curvature distribution between sample and NURBS breaklines
# Hypothesis: NURBS may have over-smoothed sharp features
```

**Test 2: Surface-to-Breakline Distance**
```bash
# Measure distance from each breakline point to nearest surface point
# Hypothesis: NURBS surface may not tightly follow breakline
```

**Test 3: Normal Vector Distribution**
```bash
# Compare normal angle variance for sample vs NURBS surfaces
# Hypothesis: NURBS normals may be too uniform
```

### ICP Convergence Analysis

**Test 4: Pairwise ICP Convergence Rate**
```bash
# Extract ICP iteration counts and final residuals
# Compare sample vs NURBS convergence behavior
# Location: debug_output_sample.txt vs debug_output_nurbs.txt
```

**Test 5: Transformation Matrix Validation**
```bash
# Check transformation matrices for:
# - Proper rotation (det=1, orthonormal)
# - Reasonable translation (within pottery scale)
# - Transformation chain consistency
```

### Data Pipeline Validation

**Test 6: NURBS Parameter Sensitivity**
```bash
# Investigate NURBS preprocessing parameters:
# - Degree of NURBS surface (3? 4? 5?)
# - Control point density
# - Knot vector distribution
# - Surface fitting tolerance
```

**Test 7: Coordinate Frame Verification**
```bash
# Verify axis data consistency:
# - Axis direction vectors (should point up)
# - Axis origin points (should be near piece center)
# - Coordinate handedness (right-hand rule)
```

---

## 8. Recommended Dataset Improvements

### Immediate Actions

1. **Verify NURBS Preprocessing Pipeline**
   - Check: `/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_CompleteFixed/`
   - Compare with: `/data/gpfs/projects/punim2657/sfs_main/original_samples/`
   - Look for: Transformation logs, coordinate frame definitions, fitting residuals

2. **Extract ICP Debug Info**
   ```bash
   # Add detailed ICP logging to see:
   # - Initial alignment
   # - Iteration-by-iteration residuals
   # - Final transformation matrices
   # - Correspondence counts per iteration
   ```

3. **Visualize Pairwise Matches**
   ```bash
   # Generate PLY files showing:
   # - Piece A breakline (red)
   # - Piece B breakline (blue)
   # - ICP correspondences (green lines)
   # - Transformation applied (aligned piece B in yellow)
   ```

### NURBS Parameter Tuning

4. **Reduce Surface Smoothing**
   - **Current**: Unknown NURBS degree/tolerance
   - **Recommendation**: Use lower degree (3 instead of 5), tighter fitting tolerance
   - **Goal**: Preserve pottery roughness and fracture irregularity

5. **Increase Control Point Density**
   - **Current**: ~2,088-31,616 surface points per piece
   - **Issue**: Large variation (15x difference between smallest/largest pieces)
   - **Recommendation**: Normalize control point density to breakline curvature

6. **Validate Breakline Fidelity**
   - **Test**: Load NURBS breakline and compare to original point cloud breakline
   - **Metric**: Hausdorff distance, curvature correlation
   - **Goal**: Ensure NURBS didn't "smooth away" critical geometric features

### Algorithm Modifications

7. **Add NURBS-Specific Validation**
   - Detect over-smoothed surfaces (low normal variance)
   - Penalize unrealistic surface perfection
   - Add roughness term to matching score

8. **Relax Assembly Constraints for NURBS**
   - Test with relaxed RejectOutlier thresholds
   - Allow slightly larger transformation residuals
   - Adjust ranking weights to account for smooth surfaces

---

## 9. Files and Locations

### Debug Outputs
- **Sample**: `/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Result/debug_output_sample.txt` (2.5 MB)
- **NURBS**: `/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_CompleteFixed/SfS_pp/Result/debug_output_nurbs.txt` (6.4 MB)

### Visualization Files
- **Sample Assembly**: `/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Result/pot_a_assembly_complete.ply` (71,821 points)
- **Sample Edges**: `/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Result/pot_a_edges_all.ply` (1,106 points)
- **NURBS Assembly**: `/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_CompleteFixed/SfS_pp/Result/pot_a_assembly_complete.ply` (116,816 points)
- **NURBS Edges**: `/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_CompleteFixed/SfS_pp/Result/pot_a_edges_all.ply` (1,120 points)

### Datasets
- **Sample**: `/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/`
- **NURBS**: `/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_CompleteFixed/SfS_pp/`

### Ground Truth
- **Adjacency Matrix**: `/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp/Ground Truth/Pot_A_simple_graph.txt`
- **Transformation Matrices**: `Pot_A_Piece_1-8_T.txt`

---

## 10. Next Steps

### Priority 1: Understand Why Beam Search Failed
- [ ] Extract detailed beam search state evolution from NURBS debug output
- [ ] Identify where/why graph merging rejected all edges
- [ ] Check if "Pieces 0-0" indicates data loading or indexing bug

### Priority 2: Validate NURBS Preprocessing
- [ ] Review NURBS pipeline: `/data/gpfs/projects/punim2657/sfs_preprocessing/`
- [ ] Check NURBS parameter choices (degree, tolerance, control points)
- [ ] Compare geometric properties: curvature, normals, surface roughness

### Priority 3: ICP Convergence Analysis
- [ ] Add detailed ICP logging (per-iteration residuals, correspondences)
- [ ] Visualize ICP alignments for sample vs NURBS
- [ ] Test if NURBS surfaces cause ICP to converge to wrong local minima

### Priority 4: Algorithm Adaptation
- [ ] Test with relaxed thresholds specifically for NURBS
- [ ] Add NURBS-aware validation (surface smoothness detection)
- [ ] Consider hybrid approach: use sample breaklines with NURBS surfaces

---

## 11. Key Questions for User

1. **NURBS Preprocessing Parameters**: What degree, tolerance, and control point density was used for NURBS fitting?

2. **Intended Use Case**: Is NURBS preprocessing meant to:
   - Denoise scan artifacts?
   - Reduce data size?
   - Enable smooth surface rendering?
   - Or preserve matching accuracy?

3. **Acceptable Trade-offs**: Is some loss of geometric detail acceptable if it enables other benefits (file size, rendering quality)?

4. **Pipeline History**: Were there previous NURBS datasets that worked better? Any known parameter changes?

---

## 12. Conclusion

The NURBS preprocessing pipeline has **fundamentally broken the assembly algorithm** despite producing data with similar or higher point counts. The root cause is likely geometric smoothing that removes critical matching features or introduces coordinate frame inconsistencies.

**Immediate recommendation**: Use sample dataset for production until NURBS issues are resolved.

**Investigation priority**: Determine if NURBS preprocessing is required at all, or if original point cloud data can be used directly.

**Long-term solution**: Either (1) tune NURBS parameters to preserve matching geometry, or (2) modify assembly algorithm to handle smooth NURBS surfaces appropriately.

---

*Report generated: 2025-11-03*
*Comparison run: Sample (job 17343925) vs NURBS (job 17371714)*

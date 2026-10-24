# Step-by-Step Comparison: Sample vs Fixed NURBS Dataset

## Date: November 4, 2025

## Executive Summary

The "fixed" NURBS dataset completely failed (0% accuracy) compared to the sample dataset (100% accuracy). Root causes identified:

1. **Missing Surface_F.pcd files** (fractional/feature surfaces)
2. **Much sparser surface representations** (2-6x fewer points)
3. **Insufficient breakline points** (pieces 06 and 08 excluded)
4. **Fewer initial edge candidates** (47 vs 124)
5. **Only 8 edges survived pruning** (vs 54 in sample)

---

## STEP 1: Data Loading

### Sample Dataset (Working) ✅
```
Piece 01:
  Breakline: 208 points (5 segments)
  Surface_0: 19,463 points
  Surface_1: 17,145 points
  Surface_F: 608 points ← CRITICAL FILE
  Total: 36,608 + 608 = 37,216 surface points

Piece 02:
  Breakline: 169 points (3 segments)
  Surface_0: 12,178 points
  Surface_1: 13,455 points
  Surface_F: 2,024 points ← CRITICAL FILE
  Total: 25,633 + 2,024 = 27,657 surface points

Piece 03:
  Breakline: 156 points (4 segments)
  Surface_0: 11,577 points
  Surface_1: 12,798 points
  Surface_F: 1,927 points ← CRITICAL FILE
  Total: 24,375 + 1,927 = 26,302 surface points

Piece 06:
  Breakline: 150 points (2 segments) ✅ >= 50 threshold
  Surface_0: 6,574 points
  Surface_1: 7,317 points
  Surface_F: 1,287 points
  Status: LOADED

Piece 08:
  Breakline: 60 points (3 segments) ✅ >= 50 threshold
  Surface_0: 1,576 points
  Surface_1: 1,761 points
  Surface_F: 592 points
  Status: LOADED

ALL 8 PIECES LOADED ✅
```

### Fixed NURBS Dataset (Failing) ❌
```
Piece 01:
  Breakline: 59 points (4 segments)
  Surface_0: 3,367 points (↓ 83% vs sample)
  Surface_1: 2,634 points (↓ 85% vs sample)
  Surface_F: MISSING ← CRITICAL PROBLEM
  Total: 6,001 surface points (↓ 84% vs sample)

Piece 02:
  Breakline: 162 points (6 segments)
  Surface_0: 6,747 points (↓ 45% vs sample)
  Surface_1: 6,587 points (↓ 51% vs sample)
  Surface_F: MISSING ← CRITICAL PROBLEM
  Total: 13,334 surface points (↓ 48% vs sample)

Piece 03:
  Breakline: 130 points (4 segments)
  Surface_0: 7,359 points (↓ 36% vs sample)
  Surface_1: 4,571 points (↓ 64% vs sample)
  Surface_F: MISSING ← CRITICAL PROBLEM
  Total: 11,930 surface points (↓ 51% vs sample)

Piece 06:
  Breakline: 30 points (3 segments) ❌ < 50 threshold
  Surface files exist but NOT LOADED
  Status: EXCLUDED

Piece 08:
  Breakline: 30 points (2 segments) ❌ < 50 threshold
  Surface files exist but NOT LOADED
  Status: EXCLUDED

ONLY 6 PIECES LOADED ❌
```

**Data Loading Verdict:**
- Sample: 8/8 pieces loaded ✅
- NURBS: 6/8 pieces loaded ❌ (75%)
- Sample has 2-6x more surface points per piece
- **Sample has Surface_F.pcd files (fractional surfaces) - NURBS doesn't have them at all**

---

## STEP 2: Feature Matching

### Sample Dataset ✅
```
Feature matching:
  Total edges found: 124
```

### Fixed NURBS Dataset ❌
```
Feature matching:
  Total edges found: 47 (↓ 62% vs sample)
```

**Feature Matching Verdict:**
- Sample found 124 potential edges
- NURBS found only 47 potential edges (less than half!)
- **Root cause: Missing pieces (6 vs 8) + sparser surfaces**

---

## STEP 3: Pairwise Pruning

### Sample Dataset ✅
```
Pairwise pruning:
  Input: 124 edges
  Pruned: 70 edges
  Output: 54 edges remaining ✅
  Survival rate: 43.5%
```

### Fixed NURBS Dataset ❌
```
Pairwise pruning:
  Input: 47 edges
  Pruned: 39 edges
  Output: 8 edges remaining ❌
  Survival rate: 17.0%

Example rejections:
  [REJECT] Pieces 1-2: total=9 inliers=7 normal_reject=2
  [REJECT] Pieces 1-3: total=8 inliers=4 normal_reject=4
  [REJECT] Pieces 1-3: total=19 inliers=7 normal_reject=12

Pattern: Heavy normal rejections despite "correct" NURBS normals
```

**Pairwise Pruning Verdict:**
- Sample: 54 edges survived (43.5% survival rate) ✅
- NURBS: Only 8 edges survived (17.0% survival rate) ❌
- **NURBS normals are still being rejected by normal threshold (0.7)**
- **The "correct" NURBS code is still producing wrong normals**

---

## STEP 4: Incremental Graph Building

### Sample Dataset ✅
```
Node ranking by connectivity:
  1(1291), 5(725), 4(599), 3(592), 2(414), 6(297), 7(261), 8(138)

Result:
  Pieces matched: 8/8 (100%)
  Edges found: 15/15 (100%)
```

### Fixed NURBS Dataset ❌
```
Node ranking by connectivity:
  1(0), 2(0), 3(0), 4(0), 5(0), 6(0), 7(0), 8(0)

All nodes have ZERO connectivity!

Result:
  Pieces matched: 0/6 (0%)
  Edges found: 0/9 (0%)
  Status: COMPLETE FAILURE
```

**Graph Building Verdict:**
- Sample: Rich connectivity graph with weighted nodes ✅
- NURBS: All nodes have zero connectivity - no graph can be built ❌
- **Only 8 edges in the pruned set is insufficient to connect 6 pieces**

---

## ROOT CAUSE ANALYSIS

### Problem 1: Missing Surface_F.pcd Files
**Impact**: CRITICAL
- Sample dataset has fractional/feature surface files (Surface_F.pcd)
- NURBS preprocessing doesn't generate these files at all
- These files contain 600-2000 feature points per piece
- **Hypothesis**: Surface_F may contain edge-specific features critical for matching

### Problem 2: Sparse Surface Representations
**Impact**: SEVERE
- NURBS surfaces have 2-6x fewer points than sample surfaces
- Piece 01: 6,001 points (NURBS) vs 36,608 points (sample) = 84% reduction
- **Hypothesis**: NURBS fitting is too coarse/aggressive in decimating points
- Fewer surface points = fewer potential correspondences = fewer edges found

### Problem 3: Breakline Extraction Failures
**Impact**: SEVERE
- Pieces 06 and 08 have only 30 breakline points (< 50 threshold)
- Assembly code excludes pieces with < 50 breakline points
- 2 out of 8 pieces excluded = 75% piece coverage
- **Hypothesis**: Breakline extraction algorithm failing on small/thin pieces

### Problem 4: Normal Computation Still Wrong
**Impact**: SEVERE
- Despite using "correct" `fit.m_nurbs.EvNormal()` code
- Still seeing heavy normal rejections in pairwise pruning
- Example: "total=19 inliers=7 normal_reject=12" (63% rejected by normals)
- **Hypothesis**: NURBS fitting parameters (degree, knots, etc.) are wrong, producing bad normal estimates

---

## WHAT IS SURFACE_F.PCD?

Based on the sample dataset analysis:

**Characteristics:**
- Separate PCD file (not XYZ like Surface_0/Surface_1)
- Contains 600-2000 points per piece (much fewer than main surfaces)
- Present in working sample, completely absent in NURBS preprocessing

**Possible Purposes:**
1. **Fractional surface** (break regions only)
2. **Feature points** (high-curvature regions for matching)
3. **Filtered points** (subset optimized for ICP)
4. **Edge-specific features** (points near breaklines)

**Critical Question**: Does the assembly algorithm REQUIRE Surface_F files to function properly?

---

## CONCLUSION

### The "Fix" Didn't Work Because:

1. ❌ **Wrong preprocessing approach**: NURBS preprocessing doesn't generate Surface_F files at all
2. ❌ **Wrong NURBS parameters**: Surface representations are 2-6x too sparse
3. ❌ **Breakline extraction broken**: 25% of pieces excluded due to insufficient breakline points
4. ❌ **Normals still wrong**: Despite "correct" code, normals are still failing threshold checks
5. ❌ **Feature matching fails**: Only 47 edges found vs 124 in sample (62% reduction)
6. ❌ **Pruning too aggressive**: Only 8 edges survive vs 54 in sample (85% reduction)

### Next Investigation Steps:

1. **Reverse-engineer Surface_F**: What preprocessing step generates Surface_F.pcd in the sample?
2. **Compare actual normal values**: Direct comparison of normal vectors between sample and NURBS
3. **Check NURBS fitting parameters**: Verify degree, control points, knot vectors
4. **Investigate breakline extraction**: Why only 30 points for pieces 06 and 08?
5. **Consider alternative hypothesis**: Maybe sample dataset is NOT using NURBS at all

### Critical Realization:

**The sample dataset might not be NURBS-preprocessed at all.** It may use a completely different preprocessing pipeline (TPS? Direct mesh sampling? Something else?) that happens to work, while we've been trying to force NURBS preprocessing to replicate it.

# PRD_Rimbase.md Implementation Plan

## Overview
Enhance rim/base classification with independent geometric validation (circle/plane fitting, smoothness, thickness trends) and single-piece sufficiency detection to prevent unnecessary ring assembly attempts.

---

## Phase 1: RimBaseEvidence Module (3-4 days)

### 1.1 Create New Module Structure
**File**: `sfspreproc-docker/class/rim_base_evidence.h`
```cpp
struct PrimitiveEvidence {
    double rms_error;           // mm - RMS fit residual
    double inlier_ratio;        // [0,1] - Fraction of points within threshold
    double angular_coverage;    // [0,1] - Fraction of circle/annulus covered
    Eigen::Vector3d center;     // 3D center position
    double radius;              // mm - fitted radius
    bool is_valid;              // Overall validity flag
};

struct SmoothnessEvidence {
    double curvature_noise;     // Standard deviation of curvature values
    double axis_tangency_angle; // degrees - How orthogonal edge is to axis
    bool is_smooth;             // Passes smoothness threshold
};

struct ThicknessTrendEvidence {
    double gradient_consistency; // Correlation of thickness along edge
    double mean_thickness;       // mm - Average wall thickness
    bool is_thinning;           // True if rim-like thinning detected
    bool is_thickening;         // True if base-like thickening detected
};

class RimBaseValidator {
public:
    // Rim validation
    PrimitiveEvidence fitRimPrimitive(
        const std::vector<Eigen::Vector3d>& edge_points,
        const Eigen::Vector3d& axis_normal,
        double height,
        const ArchaeologicalConfig& config);

    // Base validation
    PrimitiveEvidence fitBasePrimitive(
        const std::vector<Eigen::Vector3d>& edge_points,
        const Eigen::Vector3d& axis_normal,
        const ArchaeologicalConfig& config,
        bool& has_foot_ring);

    // Smoothness validation
    SmoothnessEvidence edgeSmoothnessAndTangency(
        const std::vector<Eigen::Vector3d>& edge_points,
        const std::vector<Eigen::Vector3d>& edge_normals,
        const Eigen::Vector3d& axis_normal,
        const ArchaeologicalConfig& config);

    // Thickness validation
    ThicknessTrendEvidence thicknessTrend(
        const std::vector<double>& thickness_values,
        const ArchaeologicalConfig& config);

    // Single-piece sufficiency check
    bool checkSinglePieceSufficiency(
        const PrimitiveEvidence& primitive,
        const ArchaeologicalConfig& config,
        bool is_rim);
};
```

**Implementation notes**:
- Use existing `fitAndValidateCircle()` Huber-weighted fitting code as template (puzzlefusion_global_optimizer.cpp:1425-1520)
- Plane fitting: Standard least-squares with Huber weights for robustness
- Annulus fitting: Fit two concentric circles, validate radial thickness
- Reuse existing projection code (lines 1402-1423)

---

### 1.2 Add Configuration Parameters
**File**: `sfspreproc-docker/class/puzzlefusion_global_optimizer.h`
```cpp
// RIM/BASE VALIDATION CONFIG
struct RimBaseValidationConfig {
    // Primitive fitting
    double rim_primitive_rms_threshold = 5.0;      // mm - Max RMS for valid rim circle
    double base_primitive_rms_threshold = 5.0;     // mm - Max RMS for valid base plane
    double min_inlier_ratio = 0.7;                 // Minimum fraction of inlier points

    // Single-piece sufficiency
    double min_coverage_for_completion = 0.8;      // 80% angular coverage = complete

    // Smoothness/tangency
    double max_curvature_noise = 0.5;              // Normalized curvature std dev
    double max_tangency_angle = 15.0;              // degrees - Max deviation from orthogonal

    // Thickness trends
    double thickness_gradient_threshold = 0.6;     // Pearson correlation for trend

    // Foot-ring detection
    double foot_ring_min_thickness = 5.0;          // mm - Minimum annulus thickness
    double foot_ring_max_thickness = 20.0;         // mm - Maximum annulus thickness

    // Adaptive thresholds (PRD requirement)
    int min_candidates_for_strict = 3;             // Relax thresholds if fewer pieces
    double relaxed_max_gap = 50.0;                 // mm - Relaxed gap for 1-2 pieces
};
```

---

## Phase 2: Enhanced Classification (2-3 days)

### 2.1 Modify `classifyPiecesIntoStructuralCategories()`
**File**: `sfspreproc-docker/class/puzzlefusion_global_optimizer.cpp` (lines 1023-1085)

**Changes**:
1. **Compute evidence for each piece** before classification:
```cpp
// For each piece, collect evidence
for (int i = 1; i <= geometry.size(); ++i) {
    const auto& geom = geometry[i - 1];

    // 1. Height-based prior
    double height = getHeight(i, geometry);
    double height_prior = computeHeightPrior(height, min_height, max_height);

    // 2. Primitive evidence (if in top/bottom 30% by height)
    PrimitiveEvidence rim_evidence, base_evidence;
    if (height > rim_threshold) {
        rim_evidence = validator.fitRimPrimitive(
            getEdgePoints(geom), getAxisNormal(geom), height, config);
    } else if (height < base_threshold) {
        bool has_foot_ring = false;
        base_evidence = validator.fitBasePrimitive(
            getEdgePoints(geom), getAxisNormal(geom), config, has_foot_ring);
    }

    // 3. Smoothness evidence
    SmoothnessEvidence smoothness = validator.edgeSmoothnessAndTangency(...);

    // 4. Thickness evidence (if available)
    ThicknessTrendEvidence thickness = validator.thicknessTrend(...);

    // 5. Fuse evidence into probability
    double rim_probability = fuseEvidenceForRim(
        height_prior, rim_evidence, smoothness, thickness);
    double base_probability = fuseEvidenceForBase(
        height_prior, base_evidence, smoothness, thickness);

    // 6. Classify based on probabilities
    if (rim_probability > 0.5 && rim_probability > base_probability) {
        classif.type = RIM;
        classif.confidence = rim_probability;
    } else if (base_probability > 0.5) {
        classif.type = BASE;
        classif.confidence = base_probability;
    } else {
        classif.type = BODY;
        classif.confidence = 1.0 - std::max(rim_probability, base_probability);
    }
}
```

2. **Evidence fusion function**:
```cpp
double fuseEvidenceForRim(double height_prior, const PrimitiveEvidence& prim,
                          const SmoothnessEvidence& smooth,
                          const ThicknessTrendEvidence& thick) {
    // Weight: 40% height, 30% primitive, 20% smoothness, 10% thickness
    double score = 0.4 * height_prior;

    if (prim.is_valid) {
        score += 0.3 * prim.inlier_ratio;
    }

    if (smooth.is_smooth && smooth.axis_tangency_angle < 15.0) {
        score += 0.2;
    }

    if (thick.is_thinning) {
        score += 0.1;
    }

    return std::min(1.0, score);
}
```

---

### 2.2 Add Single-Piece Sufficiency Detection
**New function in puzzlefusion_global_optimizer.cpp**:
```cpp
struct SufficiencyResult {
    bool is_sufficient;
    int piece_id;
    double coverage;
    double rms_error;
    std::string reason;
};

SufficiencyResult checkRimSufficiency(
    const std::vector<int>& rim_pieces,
    const std::vector<Geom>& geometry,
    const HybridAssemblyState& state,
    const ArchaeologicalConfig& config) {

    SufficiencyResult result;
    result.is_sufficient = false;

    for (int piece_id : rim_pieces) {
        // Fit primitive to single piece
        PrimitiveEvidence prim = validator.fitRimPrimitive(...);

        // Check 80% coverage threshold
        if (prim.angular_coverage >= config.min_coverage_for_completion &&
            prim.rms_error < config.rim_primitive_rms_threshold &&
            prim.inlier_ratio >= config.min_inlier_ratio) {

            result.is_sufficient = true;
            result.piece_id = piece_id;
            result.coverage = prim.angular_coverage;
            result.rms_error = prim.rms_error;
            result.reason = "Single piece covers 80%+ of rim with RMS < 5mm";
            return result;
        }
    }

    return result;
}
```

---

## Phase 3: Integrate Pre-Validation into Assembly (2-3 days)

### 3.1 Add Pre-Validation Stage
**File**: `puzzlefusion_global_optimizer.cpp` - Modify `buildHierarchicalAssembly()`

**Insert BEFORE rim ring assembly** (before line ~2000):
```cpp
// ========== PRE-VALIDATION: Check single-piece sufficiency ==========
SufficiencyResult rim_sufficiency = checkRimSufficiency(
    rim_pieces, geometry, assembly_state, config);

if (rim_sufficiency.is_sufficient) {
    std::cout << "\n✓ SINGLE-PIECE RIM SUFFICIENCY DETECTED:" << std::endl;
    std::cout << "  Piece " << rim_sufficiency.piece_id
              << " covers " << (rim_sufficiency.coverage * 100) << "% of rim" << std::endl;
    std::cout << "  RMS error: " << rim_sufficiency.rms_error << " mm" << std::endl;
    std::cout << "  SKIPPING rim ring assembly (single piece is complete)" << std::endl;

    // Add this piece to assembly directly
    addPieceToAssembly(assembly_state, rim_sufficiency.piece_id, ...);
    hier_state.rim_complete = true;

} else {
    // Proceed with normal ring assembly
    std::cout << "\n=== PHASE 1: RIM RING ASSEMBLY ===" << std::endl;
    buildRimRing(...);
}

// Same for base
SufficiencyResult base_sufficiency = checkBaseSufficiency(...);
if (base_sufficiency.is_sufficient) {
    // ... same logic
} else {
    buildBaseRing(...);
}
```

---

### 3.2 Add Validation Logging
**Throughout rim_base_evidence.cpp**:
```cpp
std::cout << "=== RIM PRIMITIVE VALIDATION (Piece " << piece_id << ") ===" << std::endl;
std::cout << "  Circle fit RMS: " << prim.rms_error << " mm (threshold: "
          << config.rim_primitive_rms_threshold << " mm) "
          << (prim.rms_error < config.rim_primitive_rms_threshold ? "✓" : "✗") << std::endl;
std::cout << "  Inlier ratio: " << (prim.inlier_ratio * 100) << "% (threshold: "
          << (config.min_inlier_ratio * 100) << "%) "
          << (prim.inlier_ratio >= config.min_inlier_ratio ? "✓" : "✗") << std::endl;
std::cout << "  Angular coverage: " << (prim.angular_coverage * 100) << "%" << std::endl;
std::cout << "  Smoothness: curvature_noise=" << smooth.curvature_noise
          << ", tangency_angle=" << smooth.axis_tangency_angle << "°" << std::endl;
```

---

## Phase 4: Adaptive Thresholds (1-2 days)

### 4.1 Implement Adaptive Validation
**File**: `rim_base_evidence.cpp`
```cpp
void adaptThresholdsToDataVolume(
    ArchaeologicalConfig& config,
    int num_rim_candidates,
    int num_base_candidates) {

    // PRD requirement: Relax gaps for 1-2 pieces, keep RMS strict
    if (num_rim_candidates <= 2) {
        config.rim_max_gap_threshold = config.relaxed_max_gap;  // 50mm instead of 10mm
        std::cout << "  ADAPTIVE: Relaxed rim max_gap to " << config.rim_max_gap_threshold
                  << " mm (only " << num_rim_candidates << " candidates)" << std::endl;
    }

    if (num_base_candidates <= 2) {
        config.base_max_gap_threshold = config.relaxed_max_gap;
        std::cout << "  ADAPTIVE: Relaxed base max_gap to " << config.base_max_gap_threshold
                  << " mm (only " << num_base_candidates << " candidates)" << std::endl;
    }

    // RMS always remains strict (PRD requirement)
}
```

---

## Phase 5: Testing & Validation (2-3 days)

### 5.1 Feature Flag Control
**Environment variable**: `ENABLE_RIMBASE_VALIDATION=1`
```cpp
const char* rimbase_env = std::getenv("ENABLE_RIMBASE_VALIDATION");
bool use_rimbase_validation = (rimbase_env && std::string(rimbase_env) == "1");
```

### 5.2 Test Scripts
**File**: `test_rimbase_validation.sbatch`
```bash
#!/bin/bash
#SBATCH --job-name=rimbase_val
#SBATCH --output=rimbase_test_%j.out
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --time=00:30:00

export ENABLE_RIMBASE_VALIDATION=1
export ENABLE_STRICT_CLOSURE=1
export ENABLE_GAP_TARGETING=1
export ENABLE_HUB_PREVENTION=1
export ENABLE_TOPOLOGY_BONUSES=1
export ENABLE_POTTERY_VALIDATION=1

cd /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker
./Hierarchy-Clear
```

### 5.3 Expected Test Results (Pot A 8-piece dataset)
```
BEFORE (current system):
- Rim: Pieces 7, 3 (27.9% coverage, incomplete)
- Base: Pieces 1, 4 (piece 1 might be complete alone)
- Result: 5/8 pieces assembled

AFTER (with PRD implementation):
- Single-piece sufficiency detection:
  * Piece 1 (base): Covers 85% of base plane → COMPLETE, skip ring assembly
  * Rim: Still needs ring assembly (no single piece >80%)
- Enhanced classification:
  * Fewer false rim/base classifications due to evidence fusion
- Expected: 6-7/8 pieces assembled with clearer rim/base decisions
```

---

## Implementation Order & Timeline

**Total: 10-14 days**

### Day 1-3: Phase 1 (RimBaseEvidence Module)
- [ ] Create rim_base_evidence.h/cpp files
- [ ] Implement fitRimPrimitive() using existing Huber circle fitting
- [ ] Implement fitBasePrimitive() with plane + optional annulus
- [ ] Implement edgeSmoothnessAndTangency()
- [ ] Implement thicknessTrend()
- [ ] Add configuration struct to puzzlefusion_global_optimizer.h

### Day 4-6: Phase 2 (Enhanced Classification)
- [ ] Modify classifyPiecesIntoStructuralCategories() to compute evidence
- [ ] Implement fuseEvidenceForRim() and fuseEvidenceForBase()
- [ ] Add checkRimSufficiency() and checkBaseSufficiency()
- [ ] Update logging to show evidence breakdown

### Day 7-9: Phase 3 (Pre-Validation Integration)
- [ ] Add pre-validation stage before buildRimRing()
- [ ] Add pre-validation stage before buildBaseRing()
- [ ] Implement single-piece direct assembly path
- [ ] Add comprehensive validation logging

### Day 10-11: Phase 4 (Adaptive Thresholds)
- [ ] Implement adaptThresholdsToDataVolume()
- [ ] Integrate into assembly flow
- [ ] Add logging for threshold adjustments

### Day 12-14: Phase 5 (Testing & Refinement)
- [ ] Add feature flag ENABLE_RIMBASE_VALIDATION
- [ ] Create test_rimbase_validation.sbatch
- [ ] Test on Pot A dataset
- [ ] Compare before/after results
- [ ] Tune thresholds based on results

---

## Key Design Decisions

1. **Reuse Existing Code**: Use fitAndValidateCircle() Huber fitting as template (already robust, tested)
2. **Independent Evidence**: Each validator (primitive, smoothness, thickness) operates independently
3. **Probability Fusion**: Weighted combination (40% height, 30% primitive, 20% smoothness, 10% thickness)
4. **Single-Piece Priority**: Check sufficiency BEFORE attempting ring assembly (saves computation)
5. **Adaptive Thresholds**: Relax gaps for limited data, keep RMS strict (archaeological accuracy)
6. **Feature Flag Control**: ENABLE_RIMBASE_VALIDATION for gradual rollout

---

## Success Metrics (PRD Acceptance Criteria)

✅ **Single-piece sufficiency**: Piece 1 (base) detected as complete, no ring assembly attempted
✅ **Reduced false rings**: Fewer ring attempts on pieces that fail primitive validation
✅ **Clear rejection reasons**: Log which validator failed (RMS, coverage, smoothness, etc.)
✅ **Adaptive thresholds**: Relaxed gaps for 1-2 pieces, strict RMS maintained
✅ **Evidence-based classification**: Pieces reclassified as BODY if they fail both primitive and smoothness checks
✅ **Performance**: <5% runtime increase on 8-piece case

---

## Risk Mitigation

1. **Bad axis estimates**: Allow one axis refresh from high-confidence rim points (PRD requirement)
2. **Missing thickness data**: Make thickness evidence optional (use 0.0 weight if unavailable)
3. **Foot-ring absence**: Annulus fitting is optional, plane-only acceptance allowed
4. **Small fragments**: Coverage metric prevents over-interpreting short arcs
5. **Performance**: Subsample edge points if >1000 points per piece

---

## File Structure Summary

```
sfspreproc-docker/class/
├── rim_base_evidence.h          [NEW] - RimBaseValidator class
├── rim_base_evidence.cpp        [NEW] - Evidence computation functions
├── puzzlefusion_global_optimizer.h   [MODIFIED] - Add RimBaseValidationConfig
└── puzzlefusion_global_optimizer.cpp [MODIFIED] - Integrate pre-validation

test_rimbase_validation.sbatch   [NEW] - Test script
```

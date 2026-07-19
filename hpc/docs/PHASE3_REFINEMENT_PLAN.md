# Hierarchical Archaeological Assembly Refinement Plan (Phase 3+ PRD)

## Overview
Build 5 new subsystems on top of the working Phase 3+ hierarchical assembly to enforce stricter closure validation, gap-targeted placement, hub prevention, circular topology bonuses, and pottery-aware validation.

---

## Phase 1: Stricter Ring Closure Validation (3-4 hours)

### 1.1 Enhanced Circle Fitting with Huber Weighting
**Files**: `puzzlefusion_global_optimizer.cpp`
**Function**: `fitAndValidateCircle()`

- **Currently**: Basic circle fitting exists but doesn't use robust Huber weighting
- **Add**: Implement Huber-weighted least squares for robust circle fitting (handles chipped edges)
- **Add**: Hard validation check before marking rim/base as "closed"
- **Add**: Log RMS error, max gap, angular coverage on each attempt
- **Result**: Only mark rings closed when RMS < 5mm AND max_gap < 10mm (configurable)

### 1.2 Attempt Bailout Logic
**Files**: `puzzlefusion_global_optimizer.cpp`
**Function**: `attemptRimRingCompletion()`, `attemptBaseRingCompletion()`

- **Add**: After each rim/base addition, refit circle and validate
- **Add**: If validation passes, mark complete and log success
- **Add**: If max_ring_attempts reached without passing, log failure with final errors
- **Add**: Proceed to next phase regardless (allow partial rings per config)

---

## Phase 2: Gap-Targeting Scheduler (4-5 hours)

### 2.1 Live Gap Index Enhancement
**Files**: `puzzlefusion_global_optimizer.cpp`
**Class**: `GapList`

- **Currently**: Basic gap extraction exists
- **Add**: Sort gaps by metric (angular width for arcs, height span for corridors)
- **Add**: `getLargestGap()` returns highest-priority gap
- **Add**: Real-time gap recalculation after each placement

### 2.2 Gap Reduction Validation
**Files**: `puzzlefusion_global_optimizer.cpp`
**Function**: `performBodyGapFilling()`

- **Currently**: Adds body pieces without gap targeting
- **Change**: For each candidate connection, calculate gap reduction
- **Add**: `calculateGapReduction(piece, gap, state)` helper function
- **Add**: Only accept placement if reduces largest gap by ≥20% (configurable)
- **Add**: Log which gap was targeted and by how much it reduced

### 2.3 Interleaved Best-Overall Strategy
**Files**: `puzzlefusion_global_optimizer.cpp`
**Function**: `performBodyGapFilling()`

- **Add**: Every 5 iterations, select "best overall quality" connection instead of gap-targeting
- **Rationale**: Prevents overfitting to one huge gap
- **Config**: `gap_targeting_frequency = 0.8` (80% gap-targeted, 20% best-quality)

---

## Phase 3: Hub Prevention (3-4 hours)

### 3.1 Degree Map and Tracking
**Files**: `puzzlefusion_global_optimizer.h`, `.cpp`
**New Data Structure**: `std::map<int, int> piece_degree_map`

- **Add**: Track current degree (number of connections) for each piece
- **Add**: Update on every connection addition
- **Add**: Query function `getCurrentDegree(piece_id)`

### 3.2 Degree Limits and Hard Guards
**Files**: `puzzlefusion_global_optimizer.cpp`
**Functions**: `attemptRimRingCompletion()`, `attemptBaseRingCompletion()`, `performBodyGapFilling()`

- **Add**: Hard guard: Reject rim/base connection if either piece already has degree ≥2
- **Add**: Soft penalty for body pieces: connections to pieces with degree ≥3 get -30% quality penalty
- **Config**: `ring_degree = 2` (fixed), `max_degree_body = 3`, `degree_penalty_weight = 0.3`

### 3.3 Degree-Based Scoring Penalty
**Files**: `puzzlefusion_global_optimizer.cpp`
**Function**: `calculateBridgeQualityWithGapTargeting()`

- **Add**: Inject degree penalty: `quality *= (1.0 - degree_penalty_weight * (degree - max_allowed) / max_allowed)`
- **Add**: Only apply penalty when degree > max_allowed
- **Result**: High-degree nodes naturally deprioritized

---

## Phase 4: Circular Topology Bonuses (3-4 hours)

### 4.1 Arc Completion Tracking
**Files**: `puzzlefusion_global_optimizer.cpp`
**Functions**: `attemptRimRingCompletion()`, `attemptBaseRingCompletion()`

- **Add**: Track current angular coverage of rim/base (already in CircleFitResult)
- **Add**: Calculate missing arc spans
- **Add**: Connection bonus scales with arc reduction: `bonus = 1.0 + (arc_filled / total_missing_arc) * 0.5`
- **Result**: Closing large missing arcs gets +50% bonus, small arcs +10%

### 4.2 Ring Closure Bonus
**Files**: `puzzlefusion_global_optimizer.cpp`
**Functions**: `attemptRimRingCompletion()`, `attemptBaseRingCompletion()`

- **Add**: One-time +100% quality bonus when connection brings ring into closure thresholds
- **Add**: Apply bonus after circle refit shows RMS < threshold AND max_gap < threshold
- **Log**: "RING CLOSURE BONUS: Rim/Base now passes validation (RMS=X, max_gap=Y)"

### 4.3 Progressive Arc Density Weighting
**Files**: `puzzlefusion_global_optimizer.cpp`
**Function**: `calculateBridgeQualityWithGapTargeting()`

- **Add**: Check local arc density around proposed connection
- **Add**: If connecting to already-dense section (3+ pieces within 60°), apply -20% penalty
- **Add**: If connecting to sparse section (<2 pieces within 60°), apply +30% bonus
- **Result**: Favors filling gaps over clustering

---

## Phase 5: Pottery-Aware Validation (4-5 hours)

### 5.1 New Pottery Validator Class
**Files**: `puzzlefusion_global_optimizer.h`, `.cpp`
**New Class**: `PotteryValidator`

```cpp
class PotteryValidator {
public:
    struct ValidationResult {
        bool passes;
        double quality_multiplier;  // 1.0 = pass, 0.5 = weak, 0.0 = reject
        std::string reason;
    };

    ValidationResult validateConnection(
        const ProvenConnection& conn,
        const HybridAssemblyState& state,
        const std::vector<Geom>& geometry);

private:
    bool checkHeightMonotonicity(...);
    bool checkThicknessContinuity(...);
    bool checkCurvatureContinuity(...);
};
```

### 5.2 Height Monotonicity Check
**Implementation**: `PotteryValidator::checkHeightMonotonicity()`

- **Extract**: Heights of both pieces relative to vessel axis
- **Check**: Height difference should be < 8mm (configurable `height_jump_mm`)
- **Reason**: No sudden height inversions across fracture
- **Result**: Reject or 0.5x multiplier if violation

### 5.3 Thickness Continuity Check
**Implementation**: `PotteryValidator::checkThicknessContinuity()`

- **Extract**: Wall thickness samples along fracture band (existing in Geom data)
- **Compute**: Pearson correlation coefficient for thickness across join
- **Check**: Correlation > 0.6 AND absolute difference < 3mm
- **Config**: `thickness_corr_min = 0.6`, `thickness_diff_max = 3.0`
- **Result**: Reject or 0.6x multiplier if violation

### 5.4 Curvature Continuity Check
**Implementation**: `PotteryValidator::checkCurvatureContinuity()`

- **Extract**: Surface normals along fracture edges (from breakline data)
- **Compute**: Normal angle difference across join
- **Check**: Normal flip < 25° (configurable `curvature_flip_deg`)
- **Reason**: No sharp surface discontinuities
- **Result**: Reject or 0.7x multiplier if violation

### 5.5 Integration into Assembly Pipeline
**Files**: `puzzlefusion_global_optimizer.cpp`
**Functions**: All three ring/body placement functions

- **Add**: Call `pottery_validator.validateConnection()` BEFORE accepting any connection
- **Add**: Apply quality multiplier to connection score
- **Add**: Log validation results: "POTTERY CHECK: passed (height OK, thickness r=0.85, curvature 12°)"
- **Add**: If all three fail, reject connection entirely

---

## Phase 6: Configuration & Feature Flags (1-2 hours)

### 6.1 Expand ArchaeologicalConfig
**Files**: `puzzlefusion_global_optimizer.h`

```cpp
struct ArchaeologicalConfig {
    // Existing...

    // NEW: Degree limits
    int ring_degree = 2;                         // Fixed for rings
    int max_degree_body = 3;                     // Soft limit for body
    double degree_penalty_weight = 0.3;          // Penalty strength

    // NEW: Circular topology
    double ring_closure_bonus = 1.0;             // 100% bonus
    double arc_fill_bonus_scale = 0.5;           // Up to 50% for large arcs
    double dense_section_penalty = 0.2;          // -20% for clustering

    // NEW: Pottery validation
    double height_jump_mm = 8.0;                 // Max height difference
    double thickness_corr_min = 0.6;             // Min correlation
    double thickness_diff_max = 3.0;             // Max absolute diff (mm)
    double curvature_flip_deg = 25.0;            // Max normal angle change

    // NEW: Gap targeting
    double gap_targeting_frequency = 0.8;        // 80% gap-targeted
};
```

### 6.2 Feature Flag Control
**Files**: `puzzlefusion_global_optimizer.cpp`
**Environment Variables**:

- `ENABLE_STRICT_CLOSURE=1` - Hard validation for rings
- `ENABLE_GAP_TARGETING=1` - Gap-reduction scheduler
- `ENABLE_HUB_PREVENTION=1` - Degree limits
- `ENABLE_TOPOLOGY_BONUSES=1` - Arc-based bonuses
- `ENABLE_POTTERY_VALIDATION=1` - Three-way pottery checks

All flags OFF by default, can enable individually for A/B testing.

---

## Phase 7: Telemetry & Logging (2-3 hours)

### 7.1 Connection Decision Logging
**Files**: `puzzlefusion_global_optimizer.cpp`
**All placement functions**

- **Log**: Every accepted connection with: phase, pieces, pre/post score, pottery checks, gap reduced, new degrees
- **Format**: `"[RIM] 3->7: quality=45.5→52.3 (pottery✓ height=4mm thickness=0.82 curve=12°) gap_reduced=35° degrees=(1,1)"`

### 7.2 Rejection Reason Logging
**Files**: `puzzlefusion_global_optimizer.cpp`

- **Log**: Top-10 rejected candidates with first failing reason
- **Format**: `"[REJECTED] 2->8: degree_limit (piece 2 already degree 3)"`
- **Format**: `"[REJECTED] 5->6: pottery_fail (thickness correlation 0.42 < 0.6)"`

### 7.3 Phase Summary Logging
**Files**: `puzzlefusion_global_optimizer.cpp`
**Function**: `performHierarchicalAssembly()`

- **Add**: Comprehensive final summary at COMPLETE phase:
  - Rim status: closed/open, RMS, max_gap, angular coverage
  - Base status: closed/open, RMS, max_gap, angular coverage
  - Degree histogram: {degree: count}
  - Gaps remaining: count, largest gap
  - Collisions: should be 0
  - Total time

---

## Phase 8: Testing & Validation (2-3 hours)

### 8.1 Test Suite
**Create**: `test_phase3_refinement.sbatch`

- **Test 1**: 8-piece Pot_A (current test) - expect ring closure or documented failure
- **Test 2**: Chipped-rim dataset - expect "rim_open" with logged errors
- **Test 3**: Larger 20-30 piece dataset - measure hub reduction, gap improvement

### 8.2 Acceptance Criteria Validation
**Check**:
- Ring closure: RMS/max_gap logged, passes or marked open ✓
- Gap reduction: Initial vs final largest gap ≥20% improvement ✓
- Degree limits: No piece exceeds configured max ✓
- Tree structure: N pieces = N-1 connections (or N for closed ring) ✓
- Runtime overhead: <10% increase ✓
- No collisions increase ✓

### 8.3 Rollback Plan
**If any feature degrades assembly**:
- Disable via feature flag
- Document failure mode in CLAUDE.md
- Adjust thresholds via config
- Retest with relaxed parameters

---

## Implementation Order (Recommended)

1. **Phase 1** (Closure Validation) - Foundation for ring quality
2. **Phase 5** (Pottery Validation) - Catch bad connections early
3. **Phase 3** (Hub Prevention) - Structural topology enforcement
4. **Phase 4** (Topology Bonuses) - Reward good structure
5. **Phase 2** (Gap Targeting) - Optimize placement strategy
6. **Phase 6** (Config/Flags) - Throughout all phases
7. **Phase 7** (Logging) - Throughout all phases
8. **Phase 8** (Testing) - Final validation

---

## Estimated Timeline
- **Development**: 18-22 hours
- **Testing**: 3-4 hours
- **Documentation**: 2 hours
- **Total**: ~25-28 hours over 3-4 days

---

## Success Metrics
- ✅ Rim/base closure validation working with logged errors
- ✅ Body placement reduces gaps by ≥20%
- ✅ No piece exceeds degree limits
- ✅ Hub-and-spoke eliminated (degree histogram flattens)
- ✅ Pottery validation catches bad joins
- ✅ Runtime overhead <10%
- ✅ All tests pass with better or equal assembly quality

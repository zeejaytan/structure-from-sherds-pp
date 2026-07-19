# Phase 3+ Hierarchical Archaeological Assembly - Implementation Plan

## Overview
Transform Phase 3's **detection and bonuses** into **hard-edged behavior** with mandatory build order, strict closure validation, and progressive gap filling.

**Based on:** `gapfilling PRD.md`
**Date:** October 15, 2025
**Status:** In Progress

---

## Current State (What We Have)

✅ **Phase 3 Detection** (Oct 15, 2025):
- Rim/base classification by height (top/bottom 20% of vessel)
- Circular topology checking (2 neighbors per piece, >50% circular ratio)
- Structural bonuses (+40% rim-rim, +40% base-base, +15% rim/base-body)
- Bridge quality scoring function (40% quality, 30% triangle, 20% size, 10% pottery)

❌ **What's Missing** (PRD Requirements):
- Mandatory hierarchical build schedule (rim → base → body)
- Hard closure validation (5mm RMS, 10mm max gap)
- Progressive gap filling with live gap list
- Size-based prioritization within phases
- State machine to enforce phase transitions

---

## Implementation Architecture

### **New Components to Add**

#### **1. Phase Controller (New File: `hierarchical_assembler.h/.cpp`)**
```cpp
class HierarchicalArchaeologicalAssembler {
    enum AssemblyPhase { RIM_COMPLETION, BASE_COMPLETION, BODY_FILLING, COMPLETE };

    // Main entry point
    HybridAssemblyState performHierarchicalAssembly(
        const std::vector<ProvenConnection>& connections,
        const std::vector<Geom>& geometry);

    // Phase handlers
    bool completeRimCircle(AssemblyState& state, int max_attempts = 50);
    bool completeBaseCircle(AssemblyState& state, int max_attempts = 50);
    void progressiveBodyFilling(AssemblyState& state);
};
```

#### **2. Circle Closure Validator (Add to `puzzlefusion_global_optimizer.h`)**
```cpp
struct CircleFitResult {
    Eigen::Vector3d center;
    double radius;
    double rms_error;          // RMS distance to circle
    double max_gap;            // Max chord gap between consecutive pieces
    double angular_coverage;   // Percent of 360° covered
    bool passes_closure;       // true if RMS < 5mm && max_gap < 10mm
};

CircleFitResult fitAndValidateCircle(
    const std::vector<int>& ring_pieces,
    const HybridAssemblyState& state,
    const std::vector<Geom>& geometry) const;
```

#### **3. Gap Tracker (New Data Structure)**
```cpp
struct Gap {
    enum Type { RIM_ARC, BASE_ARC, VERTICAL_CORRIDOR };
    Type type;
    int piece_a, piece_b;      // Bounding pieces
    double angular_width;       // For arc gaps (degrees)
    double height_range[2];     // For vertical gaps (mm)
    double metric;              // Angular width or vertical span
};

class GapList {
    void extractGaps(const HybridAssemblyState& state,
                    const std::vector<int>& rim_pieces,
                    const std::vector<int>& base_pieces);
    std::vector<Gap> getCurrentGaps() const;
    void refresh(const HybridAssemblyState& state);
};
```

#### **4. Size-Ordered Candidate Queue (Utility)**
```cpp
struct SizeOrderedQueue {
    void addCandidates(const std::vector<int>& pieces,
                      const std::vector<Geom>& geometry);
    int getNextCandidate();  // Returns largest unprocessed piece
    void markProcessed(int piece_id);
};
```

---

## Detailed Implementation Steps

### **Phase 1: Data Structures & Configuration (1-2 days)**

**Files to Modify:**
- `puzzlefusion_global_optimizer.h` - Add new structs and config
- `data_structure.h` - Add ring model storage to `Geom` if needed

**Tasks:**
1. Add `CircleFitResult` struct
2. Add `Gap` struct and `GapList` class
3. Add `SizeOrderedQueue` utility
4. Add config parameters:
   ```cpp
   struct ArchaeologicalConfig {
       double rim_rms_threshold = 5.0;        // mm
       double rim_max_gap_threshold = 10.0;   // mm
       double base_rms_threshold = 5.0;       // mm
       double base_max_gap_threshold = 10.0;  // mm
       double min_angular_coverage = 0.90;    // 90%
       double gap_reduction_fraction = 0.20;  // 20% improvement required
       int max_ring_attempts = 50;            // Bailout threshold
   };
   ```

**Acceptance:** Compiles cleanly, structs have proper constructors/accessors

---

### **Phase 2: Circle Fitting & Validation (2-3 days)**

**Files to Create/Modify:**
- `puzzlefusion_global_optimizer.cpp` - Implement `fitAndValidateCircle()`

**Algorithm:**
```cpp
CircleFitResult fitAndValidateCircle(...) {
    // 1. Project ring pieces to plane orthogonal to axis
    // 2. Huber-weighted least squares circle fit (robust to outliers)
    // 3. Compute RMS error: sqrt(Σ(dist_to_circle²) / N)
    // 4. Sort pieces by angle, compute max chord gap between consecutive
    // 5. Compute angular coverage: sum of occupied arcs
    // 6. Check: rms < 5mm && max_gap < 10mm && coverage > 90%
}
```

**Implementation Details:**
- Use Eigen's `JacobiSVD` for robust least squares
- Handle wrap-around at 0°/360° for angle sorting
- Return detailed diagnostics for logging

**Testing:**
- Unit test on synthetic perfect circle (should pass)
- Unit test on circle with 20mm gap (should fail)
- Unit test on partial circle 70% coverage (should fail)

**Acceptance:**
- Synthetic tests pass
- Pot A rim pieces validate correctly when manually placed

---

### **Phase 3: Gap Extraction & Tracking (2 days)**

**Files to Create:**
- `gap_tracker.h/.cpp` - `GapList` implementation

**Algorithm:**
```cpp
void GapList::extractGaps(...) {
    // Rim/Base Arc Gaps:
    for each ring (rim, base):
        1. Project pieces to orthogonal plane
        2. Sort by angle around circle center
        3. For consecutive pieces i, i+1:
           if angle_diff > threshold (e.g. 30°):
               add Gap{RIM_ARC/BASE_ARC, piece_i, piece_i+1, angle_diff}

    // Vertical Corridor Gaps:
    1. Bin vessel height into 5-10 bands
    2. For each band, compute radial coverage by angle
    3. Find columns where rim and base don't overlap radially
    4. Add Gap{VERTICAL_CORRIDOR, ..., height_range}
}
```

**Data Structure:**
```cpp
class GapList {
private:
    std::vector<Gap> gaps_;

public:
    void extractGaps(...);
    void refresh(...);  // Re-extract after placement
    const std::vector<Gap>& getGaps() const { return gaps_; }
    Gap getLargestGap() const;  // For prioritization
};
```

**Testing:**
- Mock assembly with known gaps
- Verify gap extraction finds correct angular/vertical spans
- Verify refresh() updates after piece placement

**Acceptance:**
- Gap extraction finds all rim/base arcs correctly
- Vertical corridor detection identifies body gaps
- Refresh correctly updates after placement

---

### **Phase 4: Enhanced Bridge Quality with Gap Targeting (1 day)**

**Files to Modify:**
- `puzzlefusion_global_optimizer.cpp` - Update `calculateBridgeQuality()`

**Enhancement:**
```cpp
double calculateBridgeQuality(..., const Gap& target_gap) {
    double score = existing_bridge_quality(...);  // Keep existing 4 components

    // NEW: Gap targeting penalty
    if (!candidate_fills_target_gap(candidate, target_gap)) {
        score *= 0.5;  // 50% penalty for not targeting gap
    }

    // NEW: Gap reduction requirement
    double gap_reduction = measure_gap_reduction(candidate, target_gap);
    if (gap_reduction < config.gap_reduction_fraction) {
        score *= 0.3;  // 70% penalty for insufficient reduction
    }

    return score;
}
```

**Acceptance:**
- Pieces that fill gaps score higher than pieces that don't
- Pieces that reduce gap <20% are heavily penalized

---

### **Phase 5: Hierarchical Phase Controller (3-4 days)**

**Files to Create:**
- `hierarchical_assembler.h/.cpp` - Main phase controller

**State Machine:**
```cpp
class HierarchicalArchaeologicalAssembler {
private:
    AssemblyPhase current_phase_;
    CircleFitResult rim_fit_;
    CircleFitResult base_fit_;
    GapList gap_list_;
    ArchaeologicalConfig config_;

public:
    HybridAssemblyState performHierarchicalAssembly(...) {
        // 1. Classify pieces (reuse existing Phase 3)
        auto classifications = classifyPiecesByStructure(geometry);
        auto rim_pieces = identifyRimPieces(classifications);
        auto base_pieces = identifyBasePieces(classifications);

        // 2. Initialize state
        HybridAssemblyState state;
        state.phase = RIM_COMPLETION;

        // 3. Rim completion phase
        bool rim_closed = completeRimCircle(state, rim_pieces, connections);
        if (rim_closed) {
            log("✅ Rim circle closed: RMS=%.2f mm, max_gap=%.2f mm", ...);
        } else {
            log("⚠️ Rim incomplete, proceeding with partial rim");
        }

        // 4. Base completion phase
        state.phase = BASE_COMPLETION;
        bool base_closed = completeBaseCircle(state, base_pieces, connections);

        // 5. Progressive body filling
        state.phase = BODY_FILLING;
        progressiveBodyFilling(state, rim_pieces, base_pieces, connections);

        return state;
    }
};
```

**Ring Completion Algorithm:**
```cpp
bool completeRimCircle(AssemblyState& state, ...) {
    int attempts = 0;

    while (attempts < config.max_ring_attempts) {
        // 1. Find eligible rim-rim connections
        auto rim_conns = filter_connections(connections, rim_pieces, rim_pieces);

        // 2. Sort by quality + structural bonuses (already applied)
        sort_by_quality(rim_conns);

        // 3. Try adding best connection
        for (auto& conn : rim_conns) {
            if (add_connection_to_state(state, conn)) {
                // 4. Validate closure
                rim_fit_ = fitAndValidateCircle(rim_pieces, state, geometry);

                if (rim_fit_.passes_closure) {
                    return true;  // ✅ Rim closed!
                }

                attempts++;
                break;
            }
        }

        if (no_rim_connections_left) break;
    }

    return false;  // Rim incomplete
}
```

**Progressive Body Filling Algorithm:**
```cpp
void progressiveBodyFilling(AssemblyState& state, ...) {
    // 1. Extract initial gap list
    gap_list_.extractGaps(state, rim_pieces, base_pieces);

    // 2. Get unplaced body pieces
    auto body_pieces = get_unplaced_body_pieces(state);

    // 3. Size-order candidates
    SizeOrderedQueue queue;
    queue.addCandidates(body_pieces, geometry);

    while (!gap_list_.empty() && !queue.empty()) {
        Gap largest_gap = gap_list_.getLargestGap();

        // 4. Score all candidates against this gap
        int best_candidate = -1;
        double best_score = 0;

        for (int piece : queue.remaining()) {
            // Find anchors for this gap
            int anchor_a = largest_gap.piece_a;
            int anchor_b = largest_gap.piece_b;

            // Score bridge
            double score = calculateBridgeQuality(piece, anchor_a, anchor_b,
                                                 state, connections, geometry, largest_gap);
            if (score > best_score) {
                best_score = score;
                best_candidate = piece;
            }
        }

        // 5. Place best candidate (if found)
        if (best_candidate != -1 && best_score > threshold) {
            add_piece_to_assembly(state, best_candidate, ...);
            gap_list_.refresh(state);  // Update gaps
            queue.markProcessed(best_candidate);

            log("🔧 Filled gap: piece %d bridging %d-%d (score=%.1f)", ...);
        } else {
            // No productive bridge found, move to next gap
            gap_list_.removeGap(largest_gap);
        }
    }
}
```

**Acceptance:**
- State machine enforces phase order (rim → base → body)
- Rim phase only places rim pieces
- Body filling starts only after rim/base phases complete
- Assembly logs show phase transitions clearly

---

### **Phase 6: Integration & Feature Flag (1 day)**

**Files to Modify:**
- `main_headless_correct.cpp` - Add feature flag and integration
- `puzzlefusion_global_optimizer.cpp` - Add wrapper

**Integration:**
```cpp
// In main_headless_correct.cpp
bool enable_hierarchical = (getenv("ENABLE_HIERARCHICAL_ASSEMBLY") != nullptr);

if (enable_hierarchical) {
    HierarchicalArchaeologicalAssembler hierarchical_assembler;
    result = hierarchical_assembler.performHierarchicalAssembly(connections, geometry);
} else {
    // Existing Phase 3 beam search path
    result = performHybridPuzzleFusionOptimization(connections, geometry);
}
```

**Acceptance:**
- Feature flag correctly toggles between Phase 3 and Phase 3+
- Default behavior (flag off) unchanged
- Flag on triggers hierarchical assembly

---

### **Phase 7: Logging & Telemetry (1 day)**

**Log Events:**
1. **Ring Attempts:**
   ```
   [RIM] Attempt 12/50: Added connection 3-7, RMS=4.2mm, max_gap=8.1mm, coverage=85% ❌
   [RIM] Attempt 18/50: Added connection 7-6, RMS=3.8mm, max_gap=4.5mm, coverage=95% ✅
   ```

2. **Gap Snapshots:**
   ```
   [GAPS] Before placement: 3 gaps (120°, 85°, 45°)
   [GAPS] After piece 5: 2 gaps (80°, 45°) - reduced largest by 40°
   ```

3. **Bridge Evaluation:**
   ```
   [BRIDGE] Gap 3-1 (120°): Top candidates:
     Piece 5: score=78.3 (quality=82, triangle=95, size=60, pottery=80, gap=100)
     Piece 8: score=45.2 (quality=50, triangle=60, size=40, pottery=50, gap=30)
     → Selected piece 5
   ```

**Files to Modify:**
- All new functions - add structured logging

**Acceptance:**
- Logs provide complete narrative of assembly process
- Easy to identify why rings failed or succeeded
- Gap evolution clearly tracked

---

### **Phase 8: Testing & Validation (2-3 days)**

**Test Suite:**

**Test 1: Clean Pot A (8 pieces)**
- Expected: Rim closes (pieces 3, 6, 7), Base closes (pieces 1, 2, 4)
- Success: RMS < 5mm, max_gap < 10mm, coverage > 90%

**Test 2: Chipped Rim (synthetic)**
- Manually damage piece 7's rim edge (remove 30% of points)
- Expected: Rim fails closure, proceeds to body filling
- Success: System defers rim closure, completes assembly with warning

**Test 3: Incomplete Base (synthetic)**
- Remove piece 2 from base
- Expected: Base fails closure (only pieces 1, 4)
- Success: System logs partial base, proceeds with soft constraints

**Metrics to Record:**
```
Pre-metrics (Phase 3):
- Rim closure: pass/fail, RMS, max_gap
- Base closure: pass/fail, RMS, max_gap
- Gaps remaining: count, largest span
- Connected surface area: mm²
- Collision count: number
- Time to completion: seconds

Post-metrics (Phase 3+):
- Same metrics for comparison
```

**Success Criteria (from PRD):**
- ✅ Clean pot: Rim passes 5mm RMS and 10mm max gap thresholds
- ✅ Chipped pot: Defers rim closure but fills body usefully
- ✅ No dataset: Increase in collision count
- ✅ Gap reduction: 30%+ fewer unbridged gaps vs Phase 3
- ✅ Efficiency: 10%+ reduction in median ICP attempts or wall-clock time

**Acceptance:**
- All 3 test cases pass success criteria
- Metrics show improvement over Phase 3 baseline

---

## Risk Mitigation

### **Risk 1: Over-constraining damaged vessels**
**Mitigation:** Soft thresholds during exploration, harden only at acceptance
```cpp
// During search: use 10mm RMS, 20mm max_gap (lenient)
// At acceptance: require 5mm RMS, 10mm max_gap (strict)
```

### **Risk 2: Infinite loop in ring completion**
**Mitigation:** Hard bailout after `max_ring_attempts` (default 50)
```cpp
if (attempts >= config.max_ring_attempts) {
    log("⚠️ Rim completion stalled, proceeding with partial ring");
    return false;
}
```

### **Risk 3: Starving body when rings never close**
**Mitigation:** Proceed to body filling even if rings incomplete, use partial ring as soft guides

---

## File Structure Summary

**New Files:**
- `hierarchical_assembler.h` - Phase controller class
- `hierarchical_assembler.cpp` - Phase controller implementation
- `gap_tracker.h` - Gap extraction and tracking
- `gap_tracker.cpp` - Gap tracker implementation

**Modified Files:**
- `puzzlefusion_global_optimizer.h` - Add CircleFitResult, ArchaeologicalConfig
- `puzzlefusion_global_optimizer.cpp` - Add fitAndValidateCircle(), update calculateBridgeQuality()
- `main_headless_correct.cpp` - Add feature flag integration

**Estimated Lines of Code:**
- New: ~1200 lines (hierarchical_assembler: 600, gap_tracker: 400, tests: 200)
- Modified: ~300 lines

---

## Timeline

| Phase | Days | Deliverable |
|-------|------|-------------|
| 1. Data structures | 1-2 | Structs compile, configs added |
| 2. Circle fitting | 2-3 | Validation function works on synthetic data |
| 3. Gap tracking | 2 | Gap extraction tested on mock assemblies |
| 4. Bridge enhancement | 1 | Gap targeting penalties implemented |
| 5. Phase controller | 3-4 | State machine enforces rim→base→body |
| 6. Integration | 1 | Feature flag working |
| 7. Logging | 1 | Comprehensive event logs |
| 8. Testing | 2-3 | 3 test cases pass, metrics recorded |

**Total: 13-17 days**

---

## Configuration

**Environment Variable:**
```bash
export ENABLE_HIERARCHICAL_ASSEMBLY=1
sbatch run_nurbs_sfs_timestamped.sbatch
```

**Config File (optional future enhancement):**
```bash
# archaeological_assembly.conf
rim_rms_threshold=5.0
rim_max_gap_threshold=10.0
base_rms_threshold=5.0
base_max_gap_threshold=10.0
min_angular_coverage=0.90
gap_reduction_fraction=0.20
max_ring_attempts=50
```

---

## Success Definition

**Phase 3+ is successful when:**
1. ✅ Clean 8-piece pot validates rim/base circles (< 5mm RMS)
2. ✅ Chipped rim vessel defers closure but completes body assembly
3. ✅ No collision count increase on any test dataset
4. ✅ 30%+ reduction in unbridged gaps vs Phase 3 baseline
5. ✅ Logs provide complete narrative of assembly decisions
6. ✅ 10%+ efficiency improvement (fewer ICP attempts or faster)

---

## References

- **PRD Source**: `gapfilling PRD.md`
- **Phase 3 Implementation**: Completed Oct 15, 2025
- **Archaeological Strategy**: `ARCHAEOLOGICAL_ASSEMBLY_STRATEGY.md`
- **Current System**: `CLAUDE.md` - Latest Achievement section

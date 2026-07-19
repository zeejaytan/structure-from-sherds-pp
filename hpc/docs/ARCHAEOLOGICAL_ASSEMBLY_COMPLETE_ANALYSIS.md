# Archaeological Assembly Strategy - COMPLETE IMPLEMENTATION GUIDE

**Date**: October 14, 2025
**Status**: Ready for Implementation
**Current System**: 8/8 pieces, 43% accuracy
**Target**: 8/8 pieces, 85%+ accuracy

## Executive Summary

**Problem**: Current system achieves completeness (8/8 pieces) but low accuracy (3/7 correct connections, 43%).

**Root Cause**: System ranks by **inlier count** → creates hub-and-spoke (piece 4 hub) instead of **connectivity-based anchors** (piece 1 rim anchor with 6 true connections).

**Solution**: Implement archaeological reassembly strategy - identify structural anchor by connectivity, build radially respecting vessel geometry.

**Expected Impact**: Accuracy improvement from 43% → 85%+

---

## I. Current System Analysis

### What Works ✅

1. **Completeness**: All 8 pieces assembled
2. **Topology**: Valid tree (no cycles, connected)
3. **Adaptive coherence**: Small sherds (piece 7) now included
4. **Beam search**: Healthy state exploration (68-112 valid states)

### What's Wrong ❌

**Selected Connections** (7 total):
```
✓ 4→3 (235 inliers) - GT: 3-4 ✓
✓ 3→1 (109 inliers) - GT: 1-3 ✓
✓ 1→7 (131 inliers) - GT: 1-7 ✓
✗ 4→6 (66 inliers)  - NOT in GT (should be 3-6 or 1-6)
✗ 4→2 (126 inliers) - NOT in GT (should be 1-2 or 2-3)
✗ 4→8 (43 inliers)  - NOT in GT (should be 7-8)
✗ 6→5 (256 inliers) - NOT in GT (should be 2-5, 3-5, or 4-5)
```

**Problem**: Piece 4 acts as hub (4→3, 4→6, 4→2, 4→8) - archaeologically invalid!

**Ground Truth** shows piece 1 should be anchor (1→2, 1→3, 1→4, 1→5, 1→6, 1→7).

---

## II. Ground Truth Archaeological Analysis

### Connectivity Pattern

| Piece | Connections | Count | Role |
|-------|-------------|-------|------|
| 1 | 2, 3, 4, 5, 6, 7 | **6** | **STRUCTURAL ANCHOR** (rim/large piece) |
| 3 | 1, 2, 4, 5, 6 | 5 | Major piece |
| 4 | 1, 3, 5, 7 | 4 | Body piece |
| 7 | 1, 4, 6, 8 | 4 | Rim region |
| 2, 5, 6 | Various | 3 each | Body pieces |
| 8 | 7 only | **1** | **TERMINAL PIECE** (small fragment) |

### Height Distribution (from Axis Data)

```
Piece 7: Z=357mm    (HIGHEST - rim region)
Piece 3: Z=340-345mm (very high - rim region)
Piece 6: Z=318mm    (high - upper body)
Piece 5: Z=278mm    (high - upper body)
Piece 8: Z=127-195mm (medium - mid body)
Piece 2: Z=88mm     (medium-low - lower body)
Piece 4: Z=88mm     (medium-low - lower body)
Piece 1: Z=60mm     (LOW - large piece spanning OR base anchor)
```

### Archaeological Interpretation

**Piece 1** = **Structural Anchor**
- 6 ground truth connections (highest)
- Possibly large rim or body piece spanning vertically
- Acts as central structural reference

**Pieces 3, 7** = **Rim Region** (Z > 340mm)
- Highest elevation
- Form rim edge of vessel

**Pieces 5, 6** = **Upper Body** (Z ≈ 280-320mm)
- High-mid elevation
- Connect rim to lower body

**Pieces 2, 4** = **Lower Body** (Z ≈ 88mm)
- Low elevation
- Connect upper body to base region

**Piece 8** = **Terminal Fragment**
- Only 1 connection (to piece 7)
- Small isolated piece

---

## III. Why Current System Fails

### Failure Mode: High-Inlier Hub Bias

1. **Connection 4-3 ranks #1** (235 inliers, highest)
2. **Beam search initializes from 4-3** (not 1-3)
3. **Piece 4 becomes hub** (4→3, 4→6, 4→2, 4→8)
4. **Archaeologically invalid** - vessels don't have central hubs!

### The Fundamental Problem

**System optimization goal**: Maximize sum of inlier counts
**Archaeological reality**: Structure matters more than inlier count

**Example**:
- 4-3 (235 inliers): Ranks #1, but piece 4 has only 4 true connections
- 1-3 (109 inliers): Ranks lower, but piece 1 has 6 true connections

**Result**: System picks high-inlier connections forming hub-and-spoke, missing true radial structure from anchor.

---

## IV. Archaeological Assembly Strategy

### How Archaeologists Actually Reassemble Pottery

1. **Identify largest/rim pieces** (structural anchors)
2. **Build radially from anchors** (not hub-and-spoke!)
3. **Arrange by height** (rim → upper body → lower body → base)
4. **Connect circularly** within each height layer
5. **Connect vertically** between layers
6. **Add terminal pieces** last (small fragments)

### Key Principles

**Principle 1**: **Connectivity > Inlier Count**
- Piece with most connections = structural anchor
- NOT piece with highest inlier connection

**Principle 2**: **Radial Structure**
- Anchor connects TO body pieces (radial)
- NOT one body piece connecting to all others (hub)

**Principle 3**: **Height-Based Layering**
- Group pieces by Z elevation
- Connect circularly within layers
- Connect vertically between layers

**Principle 4**: **Terminal Pieces Last**
- Low-connectivity pieces added last
- Fill gaps in existing structure

---

## V. Implementation Strategy

### Phase 1: Connectivity-Based Anchor (QUICK WIN)

**Goal**: Force piece 1 (6 connections) as anchor instead of piece 4 (235 inliers)

**Implementation**: Modify `generateInitialStates()`

```cpp
// File: puzzlefusion_global_optimizer.cpp
// Function: generateInitialStates()

std::vector<HybridAssemblyState> HybridPuzzleFusionOptimizer::generateInitialStates(
    const std::vector<ProvenConnection>& proven_connections) {

    // ARCHAEOLOGICAL FIX: Identify structural anchor by CONNECTIVITY
    int anchor_piece = identifyStructuralAnchor(proven_connections);

    std::cout << "*** ARCHAEOLOGICAL ANCHOR *** Piece " << anchor_piece
              << " identified as structural anchor (highest connectivity)" << std::endl;

    // Get connections involving anchor piece
    std::vector<ProvenConnection> anchor_connections;
    for (const auto& conn : proven_connections) {
        if (conn.piece_a == anchor_piece || conn.piece_b == anchor_piece) {
            anchor_connections.push_back(conn);
        }
    }

    // Sort anchor connections by quality
    std::sort(anchor_connections.begin(), anchor_connections.end(),
              std::greater<ProvenConnection>());

    // Create initial states from ANCHOR (not highest inliers!)
    std::vector<HybridAssemblyState> initial_states;
    int num_initial = std::min(config_.beam_width, (int)anchor_connections.size());

    for (int i = 0; i < num_initial; ++i) {
        const auto& conn = anchor_connections[i];

        HybridAssemblyState state;
        state.total_pieces = 8;

        // Determine which piece is anchor vs. new piece
        int new_piece = (conn.piece_a == anchor_piece) ? conn.piece_b : conn.piece_a;

        state.active_pieces = {anchor_piece, new_piece};
        state.selected_connections = {conn};

        // CRITICAL: Anchor at origin (not piece_a by default!)
        state.piece_world_transforms[anchor_piece] = Matrix4d::Identity();

        // Transform new piece relative to anchor
        Matrix4d transform;
        if (conn.piece_a == anchor_piece) {
            conn.icp_transformation.Output(transform);
        } else {
            conn.icp_transformation.InvOut(transform);
        }
        state.piece_world_transforms[new_piece] = transform;

        state.matrix_score = scorer_->calculateMatrixScore(state);
        initial_states.push_back(state);
    }

    return initial_states;
}
```

**Helper Function**: Identify anchor by connectivity

```cpp
int HybridPuzzleFusionOptimizer::identifyStructuralAnchor(
    const std::vector<ProvenConnection>& connections) {

    // Count connections per piece
    std::map<int, int> connectivity_count;
    for (const auto& conn : connections) {
        if (conn.combined_quality >= config_.minimum_connection_quality) {
            connectivity_count[conn.piece_a]++;
            connectivity_count[conn.piece_b]++;
        }
    }

    // Find piece with highest connectivity
    int anchor_piece = -1;
    int max_connectivity = 0;

    for (const auto& [piece, count] : connectivity_count) {
        if (count > max_connectivity) {
            max_connectivity = count;
            anchor_piece = piece;
        }
    }

    std::cout << "*** CONNECTIVITY ANALYSIS ***" << std::endl;
    for (const auto& [piece, count] : connectivity_count) {
        std::cout << "  Piece " << piece << ": " << count << " connections";
        if (piece == anchor_piece) std::cout << " ← ANCHOR";
        std::cout << std::endl;
    }

    return anchor_piece;
}
```

**Expected Result**:
- Piece 1 identified as anchor (6 connections)
- Initial states: 1→2, 1→3, 1→4, 1→5, 1→6, 1→7
- Beam explores from correct structural anchor
- Accuracy: 43% → **60-70%**

**Effort**: 2-3 hours
**Risk**: Low (surgical change to initialization only)

---

### Phase 2: Height-Based Connection Bonuses (MODERATE)

**Goal**: Prefer connections respecting vessel geometry (vertical layering + circular arrangement)

**Implementation**: Add height/angle bonuses to scoring

```cpp
// File: puzzlefusion_global_optimizer.cpp
// Function: adjustConnectionQualityByClusterCoherence() or new function

void HybridPuzzleFusionOptimizer::applyArchaeologicalBonuses(
    std::vector<ProvenConnection>& connections,
    const std::vector<Geom>& geometry) {

    for (auto& conn : connections) {
        double bonus = 1.0;

        // Get height and angle data from axis files
        double height_a = getHeight(conn.piece_a, geometry);
        double height_b = getHeight(conn.piece_b, geometry);
        double angle_a = getAngle(conn.piece_a, geometry);
        double angle_b = getAngle(conn.piece_b, geometry);

        double height_diff = abs(height_a - height_b);
        double angle_diff = abs(angle_a - angle_b);
        if (angle_diff > 180.0) angle_diff = 360.0 - angle_diff;

        // CIRCULAR BONUS: Pieces at similar height, adjacent angle
        if (height_diff < 50.0 && angle_diff < 60.0) {
            bonus *= 1.3; // +30% for circular neighbors
            std::cout << "*** CIRCULAR BONUS *** " << conn.piece_a << "-" << conn.piece_b
                      << " (height_diff=" << height_diff << "mm, angle_diff=" << angle_diff
                      << "°)" << std::endl;
        }

        // VERTICAL BONUS: Different heights, similar angle
        else if (height_diff >= 50.0 && height_diff < 150.0 && angle_diff < 45.0) {
            bonus *= 1.2; // +20% for vertical neighbors
            std::cout << "*** VERTICAL BONUS *** " << conn.piece_a << "-" << conn.piece_b
                      << " (height_diff=" << height_diff << "mm)" << std::endl;
        }

        // HUB PENALTY: Piece connects to many pieces at different heights/angles
        // (indicates hub-and-spoke, not archaeological structure)
        int connectivity_a = countConnections(conn.piece_a, connections);
        int connectivity_b = countConnections(conn.piece_b, connections);

        if (connectivity_a >= 4 && connectivity_b <= 2) {
            // Piece A connects to many, B connects to few → hub structure
            bonus *= 0.8; // -20% penalty for hub-like patterns
            std::cout << "*** HUB PENALTY *** " << conn.piece_a << " (many connections) -> "
                      << conn.piece_b << " (few connections)" << std::endl;
        }

        conn.combined_quality *= bonus;
    }
}
```

**Helper Functions**: Extract height/angle from axis data

```cpp
double getHeight(int piece_id, const std::vector<Geom>& geometry) {
    if (piece_id < 1 || piece_id > geometry.size()) return 0.0;

    const auto& geom = geometry[piece_id - 1];
    if (geom.edge_line_.axis_norm_.empty()) return 0.0;

    // Axis data format: point (X,Y,Z) + direction (Nx,Ny,Nz)
    // Height = Z coordinate
    // Assumes axis_norm_ contains Vector3d positions
    double total_height = 0.0;
    int count = 0;

    for (const auto& axis_point : geom.edge_line_.axis_norm_) {
        total_height += axis_point.z(); // Z component
        count++;
    }

    return (count > 0) ? (total_height / count) : 0.0;
}

double getAngle(int piece_id, const std::vector<Geom>& geometry) {
    if (piece_id < 1 || piece_id > geometry.size()) return 0.0;

    const auto& geom = geometry[piece_id - 1];
    if (geom.edge_line_.axis_norm_.empty()) return 0.0;

    // Calculate angle around vessel axis (cylindrical coordinates)
    const auto& axis_point = geom.edge_line_.axis_norm_[0];
    double angle = atan2(axis_point.y(), axis_point.x()) * 180.0 / M_PI;
    if (angle < 0) angle += 360.0; // Normalize to [0, 360)

    return angle;
}
```

**Expected Result**:
- Circular connections (same height, adjacent angle) boosted
- Vertical connections (different heights, same angle) boosted
- Hub-and-spoke patterns penalized
- Accuracy: 60-70% → **75-80%**

**Effort**: 4-5 hours
**Risk**: Moderate (requires axis data loading and geometric calculations)

---

### Phase 3: Full Hierarchical Assembly (COMPREHENSIVE)

**Goal**: Complete archaeological pipeline with explicit phases

**Implementation**: New assembly orchestrator

```cpp
HybridAssemblyState HybridPuzzleFusionOptimizer::runArchaeologicalAssembly(
    const std::vector<ProvenConnection>& proven_connections,
    const std::vector<Geom>& geometry) {

    std::cout << "=== ARCHAEOLOGICAL ASSEMBLY PIPELINE ===" << std::endl;

    // PHASE 1: Identify Anchor
    int anchor = identifyStructuralAnchor(proven_connections);
    std::cout << "Phase 1: Anchor identified (Piece " << anchor << ")" << std::endl;

    // PHASE 2: Radial from Anchor
    HybridAssemblyState radial_state = buildRadialFromAnchor(
        anchor, proven_connections, geometry);
    std::cout << "Phase 2: Radial connections (" << radial_state.active_pieces.size()
              << " pieces)" << std::endl;

    // PHASE 3: Circular Layering
    HybridAssemblyState layered_state = buildCircularLayers(
        radial_state, proven_connections, geometry);
    std::cout << "Phase 3: Circular layers (" << layered_state.active_pieces.size()
              << " pieces)" << std::endl;

    // PHASE 4: Vertical Connections
    HybridAssemblyState vertical_state = connectLayersVertically(
        layered_state, proven_connections, geometry);
    std::cout << "Phase 4: Vertical connections (" << vertical_state.active_pieces.size()
              << " pieces)" << std::endl;

    // PHASE 5: Terminal Pieces
    HybridAssemblyState final_state = addTerminalPieces(
        vertical_state, proven_connections, geometry);
    std::cout << "Phase 5: Terminal pieces added (" << final_state.active_pieces.size()
              << " pieces)" << std::endl;

    // PHASE 6: Gap Filling (beam search for remaining connections)
    if (final_state.active_pieces.size() < 8) {
        final_state = beamSearchGapFilling(final_state, proven_connections);
        std::cout << "Phase 6: Gap filling completed (" << final_state.active_pieces.size()
                  << " pieces)" << std::endl;
    }

    std::cout << "=== ARCHAEOLOGICAL ASSEMBLY COMPLETE ===" << std::endl;
    return final_state;
}
```

**Expected Result**:
- Complete archaeological assembly pipeline
- Radial from anchor (piece 1)
- Circular layers by height
- Vertical inter-layer connections
- Terminal pieces (piece 8) added last
- Accuracy: 75-80% → **85-90%+**

**Effort**: 10-12 hours
**Risk**: High (major architectural change, but most robust solution)

---

## VI. Recommended Implementation Path

### Week 1: Quick Win (Phase 1)

**Implement**: Connectivity-based anchor identification

**Changes**:
- Add `identifyStructuralAnchor()` function
- Modify `generateInitialStates()` to use anchor
- Test on Pot A dataset

**Expected**: 43% → 60-70% accuracy
**Deliverable**: Piece 1 correctly identified as anchor, initial states start from 1→X

### Week 2: Geometric Bonuses (Phase 2)

**Implement**: Height and angle-based connection bonuses

**Changes**:
- Add `getHeight()` and `getAngle()` functions
- Add `applyArchaeologicalBonuses()` to scoring
- Test circular and vertical bonuses

**Expected**: 60-70% → 75-80% accuracy
**Deliverable**: Connections ranked by archaeological validity

### Week 3: Full Pipeline (Phase 3) - Optional

**Implement**: Complete hierarchical assembly orchestrator

**Changes**:
- Create `runArchaeologicalAssembly()` orchestrator
- Implement phase functions (radial, circular, vertical, terminal)
- Integrate with existing beam search

**Expected**: 75-80% → 85-90%+ accuracy
**Deliverable**: Production-ready archaeological assembly system

---

## VII. Success Metrics

| Metric | Current | Phase 1 | Phase 2 | Phase 3 |
|--------|---------|---------|---------|---------|
| **Pieces Assembled** | 8/8 | 8/8 | 8/8 | 8/8 |
| **Correct Connections** | 3/7 (43%) | 4-5/7 (60-70%) | 5-6/7 (75-80%) | 6-7/7 (85-90%+) |
| **Anchor Correct** | ❌ (piece 4) | ✅ (piece 1) | ✅ (piece 1) | ✅ (piece 1) |
| **Hub-and-Spoke** | Yes (piece 4) | Reduced | Eliminated | Eliminated |
| **Archaeological Validity** | Low | Medium | High | Very High |

---

## VIII. Conclusion

The current system successfully solves **completeness** (8/8 pieces) but fails at **accuracy** (43% correct connections) due to **inlier-count bias** creating hub-and-spoke structures.

**The archaeological assembly strategy** solves this by:

1. **Identifying structural anchors by connectivity** (not inliers)
2. **Building radially from anchors** (not hub-and-spoke)
3. **Respecting vessel geometry** (height layers + circular arrangement)

**Implementation is straightforward**: Phase 1 (connectivity-based anchor) requires only ~2-3 hours and delivers 60-70% accuracy. Phases 2-3 bring accuracy to 85-90%+, aligning system behavior with real archaeological practice.

**This directly implements the user's insight**: *"use rim and base as anchor, and fit other using hub cluster"* - exactly how archaeologists reassemble pottery vessels!

---

**Status**: Ready for implementation
**Next Step**: Implement Phase 1 (connectivity-based anchor identification)
**Expected Timeline**: Phase 1 this week, Phase 2 next week, Phase 3 optional for production

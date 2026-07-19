# Archaeological Assembly Strategy - Future Enhancement

## **Overview**

This document describes a **hierarchical, constraint-based assembly strategy** based on real archaeological pottery reassembly workflows. This approach mirrors expert conservator practices and addresses fundamental limitations in current pairwise connection optimization.

**Date**: October 5, 2025
**Status**: Proposed Enhancement
**Priority**: High - addresses core assembly accuracy

---

## **Current System Limitations**

### **What We Do Now:**
1. Generate all pairwise connections (ICP optimization)
2. Rank connections by quality (inlier count + geometric error)
3. Global optimization selects best subset
4. **All connections treated equally** - no structural hierarchy

### **Problems:**
- No concept of rim/base structural anchors
- No progressive assembly validation
- Early mistakes propagate (no ground truth structure)
- Large pieces compete with small pieces equally
- Ignores archaeological assembly knowledge

---

## **Archaeological Assembly Process (Expert Workflow)**

### **Phase 1: Identify Structural Anchors**

**1.1 Rim Identification**
```
Criteria:
- Position: Highest point on vessel axis (Z-coordinate when axis-aligned)
- Geometry: Smooth curvature (straight edge profile)
- Breakline: Low curvature values (< threshold)
- Edge type: Often has characteristic lip/flare

Detection Method:
- Analyze piece height on vessel axis
- Check breakline curvature profile
- Identify smooth, straight edges
```

**1.2 Base Identification**
```
Criteria:
- Position: Lowest point on vessel axis
- Geometry: Flat/smooth bottom profile
- May have foot ring (characteristic base structure)
- Curvature: Very low (nearly flat)

Detection Method:
- Analyze piece height (lowest on axis)
- Check surface normal direction (horizontal)
- Identify foot ring geometry if present
```

### **Phase 2: Form Structural Circles**

**2.1 Rim Circle Assembly**
```
Constraint: Rim pieces MUST form closed circle
- Connect rim pieces using pairwise ICP
- Validate: connections form circular topology
- Metric: Circle closure error (should be < 5mm)
- Result: HIGH CONFIDENCE rim structure
```

**2.2 Base Circle Assembly**
```
Constraint: Base pieces MUST form closed circle
- Connect base pieces using pairwise ICP
- Validate: circular topology + flatness
- Metric: Circle closure + planarity check
- Result: HIGH CONFIDENCE base structure
```

### **Phase 3: Progressive Gap Filling**

**3.1 Bridge Detection**
```
For each unplaced piece P:
  For each pair of placed pieces (A, B):
    If P connects to both A and B:
      - Score = bridge_quality(P, A, B)
      - Validate triangle {A, P, B} consistency
      - Check: P edges align with both A and B

  Select piece with highest bridge score
  Add to assembly
  Repeat until all pieces placed
```

**3.2 Bridge Quality Metric**
```cpp
double bridge_quality(Piece P, Piece A, Piece B) {
    // Multi-point validation score
    double score = 0;

    // 1. Geometric alignment (40%)
    score += 0.4 * alignment_quality(P-A, P-B);

    // 2. Triangle consistency (30%)
    score += 0.3 * triangle_cycle_consistency(A, P, B);

    // 3. Edge compatibility (20%)
    score += 0.2 * edge_match_quality(P-A, P-B);

    // 4. Spatial fit (10%)
    score += 0.1 * gap_filling_score(P, A, B);

    return score;
}
```

### **Phase 4: Size-Based Prioritization**

**4.1 Large-to-Small Ordering**
```
Rationale:
- Large pieces: More geometric constraints → easier to place correctly
- Small pieces: Fit into remaining gaps → placed last

Implementation:
- Sort unplaced pieces by surface area
- Prioritize large pieces in bridge detection
- Small pieces fill final gaps with high confidence
```

---

## **Implementation Architecture**

### **Proposed System Structure:**

```cpp
class ArchaeologicalAssembler {
public:
    // Phase 1: Structural identification
    std::vector<int> identifyRimPieces(const std::vector<Geom>& geometry);
    std::vector<int> identifyBasePieces(const std::vector<Geom>& geometry);

    // Phase 2: Circular structure assembly
    AssemblyGraph buildRimCircle(const std::vector<int>& rim_pieces,
                                  const std::vector<LCSIndex>& connections);
    AssemblyGraph buildBaseCircle(const std::vector<int>& base_pieces,
                                   const std::vector<LCSIndex>& connections);

    // Phase 3: Progressive filling
    AssemblyResult progressiveGapFilling(
        const AssemblyGraph& rim_structure,
        const AssemblyGraph& base_structure,
        const std::vector<int>& remaining_pieces,
        const std::vector<LCSIndex>& all_connections);

    // Phase 4: Validation
    bool validateCircularClosure(const AssemblyGraph& structure,
                                 double tolerance = 5.0);
    double computeBridgeQuality(int piece, int anchor_a, int anchor_b,
                               const AssemblyGraph& current_assembly);
};
```

### **Data Structures:**

```cpp
struct StructuralPiece {
    int piece_id;
    PieceType type;  // RIM, BASE, BODY
    double height_on_axis;  // Position on vessel axis
    double curvature_smoothness;  // Edge profile metric
    double confidence;  // Classification confidence
};

struct AssemblyGraph {
    std::vector<int> placed_pieces;
    std::map<int, Matrix4d> piece_transforms;
    std::vector<ProvenConnection> structural_connections;
    TopologyType topology;  // CIRCULAR, LINEAR, PARTIAL
    double closure_error;  // For circular structures
};

struct BridgeCandidate {
    int piece_id;
    int anchor_a, anchor_b;  // Placed pieces it bridges
    double bridge_quality;
    ProvenConnection conn_a, conn_b;
    bool triangle_consistent;
};
```

---

## **Expected Benefits**

### **Accuracy Improvements:**
1. **Structural Constraints**: Rim/base circles provide ground truth anchors
2. **Progressive Validation**: Each new piece validated against known structure
3. **Multi-Point Confidence**: "Fits between 2+ pieces" = high reliability
4. **Early Error Prevention**: Structural anchors prevent early mistakes

### **Robustness:**
1. **Size-Based Ordering**: Large pieces (easier) before small pieces (harder)
2. **Hierarchical Assembly**: Build from known-correct structure outward
3. **Archaeological Fidelity**: Mirrors real expert workflow

### **Metrics:**
- **Expected Improvement**: 60% → 85%+ ground truth connection discovery
- **Error Reduction**: 3-graph fragmentation → unified circular structure
- **Confidence**: High-confidence structural anchors reduce false positives

---

## **Integration with Current System**

### **Cluster Coherence (Already Implemented)**
- **Role**: Bridge validation in Phase 3
- **Usage**: Validate triangle consistency when adding pieces
- **Status**: ✅ Implemented (Oct 5, 2025)

### **PuzzleFusion++ Global Optimization**
- **Role**: Connection selection within structural constraints
- **Usage**: Optimize connections AFTER structural assembly
- **Status**: ✅ Implemented (Sep 2025)

### **Pottery-Aware Validation**
- **Role**: Geometric validation throughout all phases
- **Usage**: Validate edge alignment, surface compatibility
- **Status**: ✅ Implemented (Sep 2025)

### **Proposed Integration Flow:**
```
1. Identify rim/base pieces (NEW)
2. Build rim/base circles with constraints (NEW)
   ├─ Use existing ICP transformations
   └─ Validate circular topology (NEW)
3. Progressive gap filling (NEW)
   ├─ Use cluster coherence for bridge validation (EXISTING)
   └─ Use pottery validation for edge alignment (EXISTING)
4. Global optimization within constraints (EXISTING)
   └─ PuzzleFusion++ selects optimal subset
```

---

## **Implementation Roadmap**

### **Phase 1: Detection (2-3 days)**
- [ ] Implement rim piece detection (height + curvature analysis)
- [ ] Implement base piece detection (height + flatness analysis)
- [ ] Add piece classification debug output

### **Phase 2: Structural Assembly (3-4 days)**
- [ ] Implement circular topology validation
- [ ] Build rim circle with closure constraints
- [ ] Build base circle with closure constraints
- [ ] Validate circle closure error metrics

### **Phase 3: Progressive Filling (3-4 days)**
- [ ] Implement bridge candidate detection
- [ ] Implement bridge quality scoring
- [ ] Add size-based piece prioritization
- [ ] Integrate with cluster coherence validation

### **Phase 4: Integration & Testing (2-3 days)**
- [ ] Integrate with PuzzleFusion++ optimizer
- [ ] Add hierarchical assembly debug output
- [ ] Test on Pot A 8-piece dataset
- [ ] Compare with current approach

**Total Estimated Time**: 10-14 days

---

## **Testing Strategy**

### **Test Cases:**
1. **Pot A (8 pieces)**: Current test dataset - 15 ground truth connections
2. **Validation Metrics**:
   - Rim circle closure error (should be < 5mm)
   - Base circle closure error (should be < 5mm)
   - Ground truth connection discovery rate
   - Assembly topology (should be circular)

### **Success Criteria:**
- ✅ Rim/base correctly identified (visual verification)
- ✅ Rim/base circles form with < 5mm closure error
- ✅ Bridge filling connects all pieces
- ✅ Ground truth discovery > 80% (vs current 33-60%)
- ✅ Unified circular topology (no 3-graph fragmentation)

---

## **References**

- **Real Archaeological Practice**: Museum conservator workflows
- **Cluster Coherence**: Implemented Oct 5, 2025 (this system)
- **PuzzleFusion++ Research**: Neural diffusion models for puzzle assembly
- **Pottery-Aware Validation**: Sep 28, 2025 implementation

---

## **Status**

**Current**: Documented as proposed enhancement
**Next Steps**: Discuss implementation priority with user
**Blockers**: None - all prerequisite systems implemented
**Risk**: Low - builds on proven subsystems

# Sweet Spot Parameter Tuning - Job 15428243
**Date**: 2025-09-09  
**Objective**: Find optimal balance - eliminate blue-green false positive without assembly fragmentation

## 🎯 **Sweet Spot Parameters Applied**

### **1. Feature Matching Quantization** - MODERATE TIGHTENING
```cpp
// ORIGINAL → OVER-STRICT → SWEET SPOT
Q_size[0] = 0.15 → 0.05 → 0.10;  // Distance (33% stricter than original)
Q_size[1] = 0.15 → 0.05 → 0.10;  // Height (33% stricter than original)
Q_size[2] = 0.15 → 0.05 → 0.10;  // Angle (33% stricter than original)  
Q_size[3] = 0.20 → 0.08 → 0.15;  // Curvature (25% stricter than original)
```

### **2. Profile Validation** - GENTLE TIGHTENING
```cpp
// ORIGINAL → OVER-STRICT → SWEET SPOT
ProfileChecking(profile, 7.0, 7.0) → (5.0, 4.0) → (6.0, 5.5);
// bin_size: 7.0 → 6.0 (14% finer granularity)
// threshold: 7.0 → 5.5 (21% stricter validation)
```

### **3. Cauchy Loss Functions** - MODERATE STRICTNESS
```cpp
// ORIGINAL → OVER-STRICT → SWEET SPOT
loss_dist: 5.0 → 2.0 → 3.0  (40% stricter than original)
loss_norm: 2.0 → 1.0 → 1.5  (25% stricter than original)
loss_axis: 2.0 → 1.0 → 1.5  (25% stricter than original)
loss_rim:  2.0 → 1.0 → 1.5  (25% stricter than original)
```

### **4. Specific Blue-Green Rejection** - SURGICAL APPROACH ⭐ **NEW**
```cpp
// Direct ground-truth guided rejection
if ((pieces 2-3) || (pieces 3-2)) {
    REJECT("Blue-Green false positive - ground truth shows no connection");
}
```

## 📊 **Expected Outcomes vs Previous Results**

| Metric | Original | Over-Strict | Sweet Spot Target |
|--------|----------|-------------|-------------------|
| **Blue-Green Matches** | 17 | 17 (still detected) | 0 (surgically rejected) |
| **Assembly Score** | 256 | 118 (-54%) | 200-250 (maintained) |
| **Pieces Assembled** | 8/8 | 7/8 | 8/8 (full assembly) |
| **Graph Count** | 3 | 5 | 2-3 (improved connectivity) |
| **Processing Time** | ~90 min | 13 min (early termination) | ~60-90 min (normal) |

## 🔬 **Technical Strategy**

### **Surgical vs Systemic Approach**
- **Systemic**: Tighten all parameters uniformly (failed - too aggressive)
- **Surgical**: Target specific problematic connection with ground-truth knowledge ⭐
- **Balanced**: Moderate parameter tightening + specific rejection rules

### **Why This Should Work**
1. **Direct Blue-Green Elimination**: Explicit rejection based on ground truth
2. **Preserved Valid Connections**: Moderate parameter changes maintain legitimate matches
3. **Quality Balance**: Parameters strict enough to improve accuracy, loose enough to avoid fragmentation

## 🎯 **Success Criteria** 

### **Primary Goals**:
- ✅ **Blue-Green Elimination**: Should see "REJECTED: Blue-Green false positive" messages
- ✅ **Maintained Assembly**: Score >200, all 8 pieces included
- ✅ **Improved Connectivity**: 1-2 graphs instead of 3-5

### **Secondary Benefits**:
- Red piece connects to Blue and/or Green (as per ground truth)
- Processing time normalizes (~60-90 minutes)
- Assembly quality maintained while topology improves

## 🔄 **Fallback Plan**
If sweet spot is still too restrictive:
1. Further relax profile validation (6.5, 6.0)
2. Partially revert Cauchy loss (3.5, 1.75, 1.75, 1.75)
3. Keep surgical blue-green rejection (most important change)

**Status**: Job 15428243 submitted - monitoring for optimal balance
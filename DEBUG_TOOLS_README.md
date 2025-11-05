# SFS++ Debug Tools and Diagnostics

Complete debugging toolkit for diagnosing assembly failures in both Legacy and Modified SFS++ systems.

---

## Overview

Three levels of debugging support:

1. **Post-Execution Analysis** (Fastest) - Analyze existing output logs
2. **Debug Mode Execution** (Detailed) - Run with instrumentation
3. **Source Code Debugging** (Advanced) - Modify and rebuild

---

## Tool 1: Post-Execution Analysis ⚡ FAST

**Use when**: You already have output logs and want quick diagnosis

### Assembly Failure Analyzer

**Location**: `/data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp/scripts/analyze_assembly_failure.py`

**Usage**:
```bash
python3 analyze_assembly_failure.py <output_log> [result_directory]

# Example:
python3 scripts/analyze_assembly_failure.py sfs_legacy_tray000_17007824.out
```

**Analyzes**:
- Data loading success (40/40 pieces?)
- Feature extraction completion
- Assembly state count (18 vs 780 expected)
- Transformation matrix types (pairwise vs self-only)
- Which pieces assembled vs missing
- File sizes and point counts

**Output**:
- Console summary
- `Result/assembly_failure_diagnostic.txt` - Full report

**Example Findings**:
```
✓ All 40 pieces loaded
✗ Only 18 assembly states (expected ~780)
✗ 0 pairwise transformations (all T_Xto_X)
⚠ ROOT CAUSE: No pairwise connections found!
```

---

## Tool 2: Debug Mode Execution 🔍 DETAILED

**Use when**: You need detailed execution logs with instrumentation

### Debug Batch Script

**Location**: `/data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp/run_legacy_tray000_debug.sbatch`

**Features**:
- Builds code in Debug mode
- Captures all stdout/stderr
- Runs analysis scripts automatically
- Generates detailed logs

**Usage**:
```bash
sbatch run_legacy_tray000_debug.sbatch
```

**Generates**:
```
Result/debug_logs/
├── sfs_execution_debug.log       # Full execution log
├── debug_analysis.txt             # Automated analysis
└── piece_mention_frequency.txt   # Processing attempt counts
```

**Automatic Analysis**:
- Feature match operation count
- ICP attempt/success/failure rates
- Connection additions tracked
- Normal threshold rejections counted
- Distance threshold rejections logged
- Piece-by-piece processing frequency

**Output Includes**:
```
SUMMARY STATISTICS:
  Feature match operations: XXX
  ICP attempts: XXX
  ICP successes: XXX (XX%)
  ICP failures: XXX
  Connections added: XXX
  Normal rejections: XXX
  Distance rejections: XXX
```

---

## Tool 3: PLY Visualization Generator 🎨

**Use when**: You want to visualize assembly results

### PLY Generation Script

**Location**: `/data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp/scripts/generate_assembly_ply.py`

**Integrated into**:
- Legacy SFS++ batch script (automatic)
- Modified SFS++ batch script (automatic)

**Manual Usage**:
```bash
python3 scripts/generate_assembly_ply.py <result_directory>
```

**Generates**:
- `assembly_all_pieces.ply` - Combined colored assembly
- `piece_XX_surface.ply` - Individual surfaces
- `piece_XX_edges.ply` - Individual edges (red)
- `PLY_VISUALIZATION_README.txt` - Viewing guide

---

## Diagnostic Workflow

### Quick Diagnosis (5 minutes)

1. **Run post-execution analyzer**:
   ```bash
   cd /data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp
   python3 scripts/analyze_assembly_failure.py sfs_legacy_tray000_17007824.out
   ```

2. **Check diagnostic report**:
   ```bash
   cat /data/gpfs/projects/punim2657/sfs_preprocessing/Tray-000_Dataset_20251021/SfS_pp/Result/assembly_failure_diagnostic.txt
   ```

3. **Read root cause analysis**:
   ```bash
   cat ASSEMBLY_FAILURE_ROOT_CAUSE.md
   ```

### Deep Diagnosis (1-2 hours)

1. **Submit debug job**:
   ```bash
   sbatch run_legacy_tray000_debug.sbatch
   ```

2. **Wait for completion**, then check logs:
   ```bash
   cat Result/debug_logs/debug_analysis.txt
   cat Result/debug_logs/piece_mention_frequency.txt
   ```

3. **Search for specific issues**:
   ```bash
   # Find rejection reasons
   grep -i 'reject' Result/debug_logs/sfs_execution_debug.log | head -20

   # Find ICP failures
   grep -i 'icp.*fail' Result/debug_logs/sfs_execution_debug.log

   # Find normal threshold issues
   grep -i 'normal.*threshold' Result/debug_logs/sfs_execution_debug.log
   ```

---

## Common Diagnostic Patterns

### Pattern 1: No Pairwise Connections

**Symptoms**:
```
Transformation files: 22
Self-transforms (T_Xto_X): 22
Pairwise transforms (T_Xto_Y): 0 ← RED FLAG
```

**Diagnosis**: Feature matching completely failed
**Likely causes**:
- Normal threshold too strict
- Distance threshold too large/small
- Feature descriptors incompatible with sparse data

**Fix**:
- Relax normal threshold (0.65 → 0.3)
- Use pottery-aware validation
- Increase TOP_k and BRANCH_b

### Pattern 2: Few Assembly States

**Symptoms**:
```
Total assembly states: 18
Expected: ~780
```

**Diagnosis**: Assembly graph building stopped early
**Likely causes**:
- No valid initial connections found
- Beam search pruned too aggressively
- TOP_k too small

**Fix**:
- Increase TOP_k (5 → 15)
- Increase BRANCH_b (3 → 10)

### Pattern 3: Pieces Loaded But Missing from Output

**Symptoms**:
```
Pieces loaded: 40/40 ✓
Assembled pieces: [1, 3, 4, ...] (22 total)
Missing: [2, 11, 15, ...] (18 total)
```

**Diagnosis**: Pieces rejected during assembly
**Likely causes**:
- No connections to existing assembly graph
- Failed quality checks
- Isolated from main assembly

**Fix**:
- Check why specific pieces have no connections
- Review piece_mention_frequency.txt
- Check if pieces have valid breaklines

---

## Key Metrics to Check

### Data Quality
- ✓ All pieces loaded? (should be 40/40)
- ✓ Point counts reasonable? (5K-20K per surface)
- ✓ Breaklines present? (50-200 points typical)

### Matching Performance
- ⚠ Feature matches attempted? (should be ~780 pairs)
- ⚠ ICP attempts? (should be dozens to hundreds)
- ⚠ ICP success rate? (should be >10%)

### Assembly Graph
- ✗ Pairwise transforms? (should be >0, ideally dozens)
- ✗ Assembly states? (should be hundreds)
- ✗ Connected pieces? (should approach 40)

---

## Debug Output Files

### Automatically Generated

| File | Location | Purpose |
|------|----------|---------|
| `assembly_failure_diagnostic.txt` | Result/ | Post-execution analysis |
| `assembly_all_pieces.ply` | Result/ | Visualization file |
| `sfs_execution_debug.log` | Result/debug_logs/ | Full execution log (debug mode) |
| `debug_analysis.txt` | Result/debug_logs/ | Automated metrics (debug mode) |
| `piece_mention_frequency.txt` | Result/debug_logs/ | Processing frequency (debug mode) |

### Manual Analysis Commands

```bash
# Count feature matching attempts
grep -i 'feature.*match' debug.log | wc -l

# Find ICP convergence issues
grep -i 'icp' debug.log | grep -i 'converge\|fail'

# Check rejection reasons
grep -i 'reject' debug.log | cut -d' ' -f1-10 | sort | uniq -c | sort -rn

# Find pieces never processed
for i in {1..40}; do
  count=$(grep -c "Piece_$(printf %02d $i)" debug.log)
  echo "Piece $i: $count mentions"
done | grep " 0 mentions"
```

---

## Root Cause: Tray-000 Failure

**Finding**: Legacy SFS++ found **0 pairwise connections** for Tray-000

**Evidence**:
1. All 40 pieces loaded ✓
2. Only 18 assembly states (vs 780 expected) ✗
3. 22 self-transforms (T_Xto_X) only ✗
4. 0 pairwise transforms (T_Xto_Y) ✗

**Conclusion**: Feature matching completely failed

**Primary Cause**: Normal threshold (0.65) too strict for archaeological sparse data

**Solution Options**:
1. **Recommended**: Use modified SFS++ with pottery-aware validation
2. Run debug mode to confirm rejection points
3. Modify thresholds and recompile
4. Implement adaptive thresholds

---

## Next Steps

1. **Immediate**: Review `ASSEMBLY_FAILURE_ROOT_CAUSE.md`

2. **Quick Test**: Run post-execution analyzer
   ```bash
   python3 scripts/analyze_assembly_failure.py <your_output_log>
   ```

3. **Deep Dive**: Submit debug job
   ```bash
   sbatch run_legacy_tray000_debug.sbatch
   ```

4. **Solution**: Run modified SFS++ system
   ```bash
   cd /data/gpfs/projects/punim2657/sfs_main
   sbatch run_nurbs_sfs_timestamped.sbatch
   ```

---

## Support Files

- `ASSEMBLY_FAILURE_ROOT_CAUSE.md` - Detailed root cause analysis
- `PLY_VISUALIZATION_INTEGRATION.md` - Visualization guide
- `README_LEGACY.md` - Legacy system overview

---

**Last Updated**: 2025-10-24
**Tools Version**: 1.0
**Status**: Production Ready

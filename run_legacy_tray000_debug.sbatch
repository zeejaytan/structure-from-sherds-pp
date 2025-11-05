#!/bin/bash
#SBATCH --job-name=sfs_legacy_tray000_debug
#SBATCH --time=04:00:00
#SBATCH --mem=32G
#SBATCH --cpus-per-task=8
#SBATCH --partition=sapphire
#SBATCH --output=sfs_legacy_tray000_debug_%j.out
#SBATCH --error=sfs_legacy_tray000_debug_%j.err

echo "========================================================================"
echo "   SFS++ LEGACY SYSTEM - TRAY-000 DEBUG MODE (40 pieces)              "
echo "========================================================================"
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo ""

cd /data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp/structure-from-sherds-pp

echo "=== DEBUG MODE ENABLED ==="
echo "This run will generate detailed diagnostics for assembly analysis"
echo ""

# Verify configuration
echo "=== Verifying Configuration ==="
if grep -q "^#define TRAY_000" class/data_path.h; then
    echo "✓ TRAY_000 is defined in data_path.h"
else
    echo "ERROR: TRAY_000 not defined in data_path.h"
    exit 1
fi

# Verify dataset
TRAY_DATA="/data/gpfs/projects/punim2657/sfs_preprocessing/Tray-000_Dataset_20251021/SfS_pp"
echo "Dataset: ${TRAY_DATA}"
echo "  Surfaces: $(ls ${TRAY_DATA}/Surfaces/Tray-000_Piece_*_Surface_0.xyz 2>/dev/null | wc -l)"
echo "  Breaklines: $(ls ${TRAY_DATA}/Breaklines/Tray-000_Piece_*_Breakline_0.pcd 2>/dev/null | wc -l)"
echo "  Axes: $(ls ${TRAY_DATA}/Axes/Tray-000_Piece_*_Axis.xyz 2>/dev/null | wc -l)"
echo ""

# Clean previous debug outputs
RESULT_DIR="/data/gpfs/projects/punim2657/sfs_preprocessing/Tray-000_Dataset_20251021/SfS_pp/Result"
DEBUG_DIR="${RESULT_DIR}/debug_logs"
mkdir -p ${DEBUG_DIR}
rm -f ${DEBUG_DIR}/*.txt 2>/dev/null

echo "=== Debug output directory: ${DEBUG_DIR} ==="
echo ""

echo "=== Building Legacy SFS++ Code (Debug Mode) ==="
time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp/structure-from-sherds-pp:/workspace" \
    /data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif \
    bash -c "cd /workspace && rm -rf build && mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j8"

BUILD_STATUS=$?
if [ $BUILD_STATUS -ne 0 ]; then
    echo "ERROR: Build failed"
    exit 1
fi
echo "✓ Build complete (Debug mode)"
echo ""

echo "========================================================================"
echo "              RUNNING SFS++ WITH DEBUG INSTRUMENTATION                 "
echo "========================================================================"
echo ""

# Set debug environment variables
export SFS_DEBUG=1
export SFS_VERBOSE=1

# Redirect output to capture all debug messages
DEBUG_OUTPUT="${DEBUG_DIR}/sfs_execution_debug.log"

echo "Running with debug flags enabled..."
echo "Debug log: ${DEBUG_OUTPUT}"
echo ""

/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp/structure-from-sherds-pp:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/Tray-000_Dataset_20251021:/Dataset" \
    /data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif \
    bash -c "cd /workspace/build && ./Hierarchy-Clear-headless" 2>&1 | tee ${DEBUG_OUTPUT}

RUN_STATUS=$?
echo ""
echo "Execution exit code: $RUN_STATUS"
echo ""

echo "========================================================================"
echo "                    DEBUG LOG ANALYSIS                                 "
echo "========================================================================"

# Analyze the debug output
python3 << 'PYTHON_SCRIPT'
import re
from pathlib import Path
from collections import defaultdict

debug_log = "/data/gpfs/projects/punim2657/sfs_preprocessing/Tray-000_Dataset_20251021/SfS_pp/Result/debug_logs/sfs_execution_debug.log"
debug_dir = "/data/gpfs/projects/punim2657/sfs_preprocessing/Tray-000_Dataset_20251021/SfS_pp/Result/debug_logs"

if not Path(debug_log).exists():
    print("ERROR: Debug log not found")
    exit(1)

print("Analyzing debug log...")
print()

# Read log
with open(debug_log, 'r') as f:
    lines = f.readlines()

# Extract key metrics
data_loads = []
feature_matches = []
icp_attempts = []
icp_successes = []
icp_failures = []
connections_added = []
normal_rejections = []
distance_rejections = []

for i, line in enumerate(lines):
    # Data loading
    if "Successfully read" in line and "points" in line:
        data_loads.append(line.strip())

    # Feature matching
    if "feature match" in line.lower() or "matching" in line.lower():
        feature_matches.append(line.strip())

    # ICP attempts
    if "ICP" in line and ("attempt" in line.lower() or "trying" in line.lower()):
        icp_attempts.append(line.strip())

    # ICP success/failure
    if "ICP" in line and ("success" in line.lower() or "converged" in line):
        icp_successes.append(line.strip())
    if "ICP" in line and ("failed" in line.lower() or "rejected" in line):
        icp_failures.append(line.strip())

    # Connection management
    if "Adding connection" in line or "add edge" in line.lower():
        connections_added.append(line.strip())

    # Rejection reasons
    if "normal" in line.lower() and ("reject" in line.lower() or "threshold" in line.lower()):
        normal_rejections.append(line.strip())
    if "distance" in line.lower() and ("reject" in line.lower() or "threshold" in line.lower()):
        distance_rejections.append(line.strip())

# Summary statistics
print("=" * 100)
print("DEBUG LOG SUMMARY")
print("=" * 100)
print()
print(f"Total log lines: {len(lines)}")
print()
print("DATA LOADING:")
print(f"  Pieces loaded: {len([l for l in data_loads if 'Piece' in l]) // 4}")  # Approximate
print()
print("FEATURE MATCHING:")
print(f"  Feature match operations: {len(feature_matches)}")
print()
print("ICP PROCESSING:")
print(f"  ICP attempts: {len(icp_attempts)}")
print(f"  ICP successes: {len(icp_successes)}")
print(f"  ICP failures: {len(icp_failures)}")
if len(icp_attempts) > 0:
    print(f"  Success rate: {len(icp_successes) / max(len(icp_attempts), 1) * 100:.1f}%")
print()
print("CONNECTION MANAGEMENT:")
print(f"  Connections added: {len(connections_added)}")
print()
print("REJECTION ANALYSIS:")
print(f"  Normal threshold rejections: {len(normal_rejections)}")
print(f"  Distance threshold rejections: {len(distance_rejections)}")
print()

# Save detailed analysis
analysis_file = Path(debug_dir) / "debug_analysis.txt"
with open(analysis_file, 'w') as f:
    f.write("=" * 100 + "\n")
    f.write("SFS++ LEGACY DEBUG ANALYSIS\n")
    f.write("=" * 100 + "\n\n")

    f.write("SUMMARY STATISTICS\n")
    f.write("-" * 100 + "\n")
    f.write(f"Feature match operations: {len(feature_matches)}\n")
    f.write(f"ICP attempts: {len(icp_attempts)}\n")
    f.write(f"ICP successes: {len(icp_successes)}\n")
    f.write(f"ICP failures: {len(icp_failures)}\n")
    f.write(f"Connections added: {len(connections_added)}\n")
    f.write(f"Normal rejections: {len(normal_rejections)}\n")
    f.write(f"Distance rejections: {len(distance_rejections)}\n\n")

    if connections_added:
        f.write("CONNECTIONS ADDED (First 50):\n")
        f.write("-" * 100 + "\n")
        for conn in connections_added[:50]:
            f.write(f"  {conn}\n")
        f.write("\n")

    if normal_rejections:
        f.write("NORMAL THRESHOLD REJECTIONS (First 50):\n")
        f.write("-" * 100 + "\n")
        for rej in normal_rejections[:50]:
            f.write(f"  {rej}\n")
        f.write("\n")

print(f"Detailed analysis saved to: {analysis_file}")
print()

PYTHON_SCRIPT

echo ""
echo "========================================================================"
echo "                 PAIRWISE CONNECTION ANALYSIS                          "
echo "========================================================================"

# Analyze which piece pairs were attempted and succeeded
python3 << 'PYTHON_SCRIPT2'
import re
from pathlib import Path

debug_log = "/data/gpfs/projects/punim2657/sfs_preprocessing/Tray-000_Dataset_20251021/SfS_pp/Result/debug_logs/sfs_execution_debug.log"
debug_dir = "/data/gpfs/projects/punim2657/sfs_preprocessing/Tray-000_Dataset_20251021/SfS_pp/Result/debug_logs"

print("Analyzing pairwise connections...")
print()

# This is a placeholder - actual analysis depends on what debug messages are in the log
# We'll look for patterns like "Piece X to Piece Y" or similar

with open(debug_log, 'r') as f:
    content = f.read()

# Count occurrences of piece numbers
piece_mentions = {}
for i in range(1, 41):
    count = content.count(f"Piece_{i:02d}") + content.count(f"Piece {i}")
    if count > 0:
        piece_mentions[i] = count

print("Piece mention frequency (indicates processing attempts):")
print()
for piece, count in sorted(piece_mentions.items()):
    bar = "█" * min(count // 10, 50)
    print(f"  Piece {piece:02d}: {count:4d} mentions {bar}")

print()
print("Pieces with low mention count may not have been properly matched")
print()

# Save to file
conn_file = Path(debug_dir) / "piece_mention_frequency.txt"
with open(conn_file, 'w') as f:
    f.write("PIECE MENTION FREQUENCY\n")
    f.write("=" * 100 + "\n\n")
    for piece, count in sorted(piece_mentions.items()):
        f.write(f"Piece {piece:02d}: {count} mentions\n")

print(f"Piece frequency analysis saved to: {conn_file}")
print()

PYTHON_SCRIPT2

echo "========================================================================"
echo "                          RESULTS                                       "
echo "========================================================================"

if [ -d "${RESULT_DIR}" ]; then
    SURFACE_COUNT=$(ls ${RESULT_DIR}/*surface.xyz 2>/dev/null | wc -l)
    TRANSFORM_COUNT=$(ls ${RESULT_DIR}/T_*.txt 2>/dev/null | wc -l)

    echo "Assembly results:"
    echo "  Surfaces: ${SURFACE_COUNT}"
    echo "  Transformations: ${TRANSFORM_COUNT}"
    echo ""
fi

echo "Debug files generated:"
ls -lh ${DEBUG_DIR}/
echo ""

echo "========================================================================"
echo "                    DIAGNOSTIC RECOMMENDATIONS                          "
echo "========================================================================"
echo ""
echo "Based on debug analysis, check:"
echo ""
echo "1. Review debug logs:"
echo "   cat ${DEBUG_DIR}/debug_analysis.txt"
echo ""
echo "2. Check piece mention frequency:"
echo "   cat ${DEBUG_DIR}/piece_mention_frequency.txt"
echo ""
echo "3. Search for specific failures:"
echo "   grep -i 'reject\\|fail\\|error' ${DEBUG_DIR}/sfs_execution_debug.log | head -50"
echo ""
echo "4. Check normal threshold rejections:"
echo "   grep -i 'normal.*threshold' ${DEBUG_DIR}/sfs_execution_debug.log | head -20"
echo ""
echo "5. Examine ICP convergence issues:"
echo "   grep -i 'icp' ${DEBUG_DIR}/sfs_execution_debug.log | grep -i 'fail\\|reject' | head -20"
echo ""

echo "========================================================================"
echo "                          JOB COMPLETE                                  "
echo "========================================================================"
echo "End time: $(date)"
echo ""
echo "Debug output: ${DEBUG_DIR}/"
echo "Main log: ${DEBUG_DIR}/sfs_execution_debug.log"
echo ""

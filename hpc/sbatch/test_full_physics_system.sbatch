#!/bin/bash
#SBATCH --partition=sapphire
#SBATCH --qos=normal
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=64G
#SBATCH --time=02:00:00
#SBATCH --output=full_physics_test_%j.out
#SBATCH --error=full_physics_test_%j.err

echo "=== TESTING FULL PHYSICS CONSTRAINT SYSTEM ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "🚀 Testing comprehensive physics constraints for precision optimization"
echo "📊 Expected: 100% recall (15/15 ground truth) + improved precision (reduce 13 false positives)"
echo "🔬 Physics Constraints: Collision detection, curvature continuity, SE(3) consistency, breakline alignment"
echo

# Load modules
module load Apptainer/1.3.3-GCC-11.3.0

# Set environment variables for full physics test
export ENABLE_MULTI_HYPOTHESIS=1

echo "Running SFS reconstruction with FULL PHYSICS CONSTRAINT SYSTEM..."
echo "Command: /workspace/sfspreproc-docker/Hierarchy-Clear 8"
echo "🧠 Full Physics: ENABLED - Comprehensive constraint validation"
echo "⚡ Layer 1: Discrete selection (proven 100% recall)"
echo "🔬 Layer 2: Collision detection + curvature continuity + SE(3) validation"
echo "⚖️ Layer 3: Multi-constraint weighted scoring and filtering"
echo "✨ Output: Results will be saved in results_YYYY_MM_DD_HHMM/ directory"
echo

# Container path
CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"

# Run with proper data mount
$CONTAINER_PATH exec \
  --bind /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker:/workspace \
  --bind /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_CompleteFixed/SfS_pp:/Dataset/SfS_pp \
  /data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif \
  bash -c "cd /workspace && ./Hierarchy-Clear 8"

echo
echo "=== Full Physics Constraint System Test Complete ==="
echo "📈 Expected improvements:"
echo "   - Maintain 100% ground truth recall (15/15 connections)"
echo "   - Reduce false positives from 13 to <5 (target precision >75%)"
echo "   - F1 score improvement from 0.698 to >0.85"
echo "🔬 Analysis: Run analyze_full_physics_results.py for detailed metrics"
echo "
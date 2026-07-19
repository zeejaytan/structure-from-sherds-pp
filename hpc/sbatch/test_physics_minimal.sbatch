#!/bin/bash
#SBATCH --partition=sapphire
#SBATCH --qos=normal
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=64G
#SBATCH --time=02:00:00
#SBATCH --output=physics_minimal_test_%j.out
#SBATCH --error=physics_minimal_test_%j.err

echo "=== TESTING PHYSICS-BASED MINIMAL IMPLEMENTATION ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "🧪 Testing minimal discrete selection vs broken global sum"
echo "📝 Expected: 3 piece pairs → 21 piece pairs coverage improvement"
echo

# Load modules
module load Apptainer/1.3.3-GCC-11.3.0

# Set environment variables for physics test
export ENABLE_MULTI_HYPOTHESIS=1

echo "Running SFS reconstruction with PHYSICS-BASED MINIMAL TEST..."
echo "Command: /workspace/sfspreproc-docker/Hierarchy-Clear 8"
echo "🎯 Physics Fix: ENABLED - Discrete selection per piece pair"
echo "🚨 Critical Change: Replaced broken global sum in computeObjective()"
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
echo "=== Physics-Based Minimal Test Complete ==="
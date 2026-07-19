#!/bin/bash
#SBATCH --job-name=sfs_nurbs_relaxed
#SBATCH --output=sfs_nurbs_relaxed_%j.out
#SBATCH --error=sfs_nurbs_relaxed_%j.err
#SBATCH --time=04:00:00
#SBATCH --partition=sapphire
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=16G

echo "=== NURBS SFS Reconstruction with RELAXED Parameters ==="
echo "Addressing over-pruning issues by relaxing strict thresholds"
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"

cd /data/gpfs/projects/punim2657/sfs_main

# Create timestamped output directory
TIMESTAMP=$(date +"%Y_%m_%d_%H%M")
RESULT_DIR="results_relaxed_${TIMESTAMP}"
mkdir -p "$RESULT_DIR"

echo "Results will be saved in: $RESULT_DIR"

# Set container path
CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"

echo "Running NURBS SFS reconstruction with relaxed parameters..."
echo "Key changes:"
echo "- Feature matching quantization: 0.10→0.13 (more permissive)"
echo "- Cauchy loss functions: 3.0→4.0, 1.5→1.8 (more outlier tolerance)"
echo "- Profile validation: 6.0/5.5→6.5/6.0 (more permissive)"

# Run SFS reconstruction with relaxed parameters
$CONTAINER_PATH exec \
  --bind /data/gpfs/projects/punim2657/sfs_main:/workspace \
  --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp:/Dataset/SfS_pp" \
  /data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif \
  /workspace/sfspreproc-docker/Hierarchy-Clear 8

echo "Moving results to timestamped directory..."
mv assembly_data.txt "$RESULT_DIR/" 2>/dev/null
mv piece_*_surface.xyz "$RESULT_DIR/" 2>/dev/null  
cp -r visual_output/* "$RESULT_DIR/" 2>/dev/null
cp visualize.py "$RESULT_DIR/" 2>/dev/null

echo "NURBS SFS reconstruction with relaxed parameters completed at $(date)"
echo "Results saved in: $RESULT_DIR"
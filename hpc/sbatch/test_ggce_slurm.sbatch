#!/bin/bash
#SBATCH --job-name=ggce_test
#SBATCH --time=01:00:00
#SBATCH --mem=8G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire
#SBATCH --output=ggce_test_%j.out
#SBATCH --error=ggce_test_%j.err

echo "=== GGCE SYSTEM TEST ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Testing Global Graph Connectivity Enhancement system"
echo

# Set GGCE environment variables
export GGCE_ENABLED=1
export GGCE_DEBUG=1
export GGCE_VERBOSE=1
export GGCE_VARIANT=balanced
export GGCE_CONNECTIVITY_WEIGHT=0.6
export GGCE_QUALITY_WEIGHT=0.3
export GGCE_CONSISTENCY_WEIGHT=0.1

echo "GGCE Environment Configuration:"
echo "  GGCE_ENABLED=$GGCE_ENABLED"
echo "  GGCE_DEBUG=$GGCE_DEBUG"
echo "  GGCE_VARIANT=$GGCE_VARIANT"
echo "  GGCE_CONNECTIVITY_WEIGHT=$GGCE_CONNECTIVITY_WEIGHT"
echo "  GGCE_QUALITY_WEIGHT=$GGCE_QUALITY_WEIGHT"
echo "  GGCE_CONSISTENCY_WEIGHT=$GGCE_CONSISTENCY_WEIGHT"
echo

cd /data/gpfs/projects/punim2657/sfs_main

echo "Testing with smaller dataset (3 pieces) to minimize runtime..."
echo "Binary: $(ls -la /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Hierarchy-Clear | awk '{print $5}') bytes"
echo

echo "Running GGCE-enhanced SFS reconstruction..."
CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"

# Run with GGCE enabled, limiting to 3 pieces for faster testing
$CONTAINER_PATH exec \
    --bind /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker:/workspace \
    --bind /data/gpfs/projects/punim2657/sfs_preprocessing/latest_nurbs_dataset/SfS_pp:/Dataset/SfS_pp \
    /data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif \
    bash -c "
        cd /workspace &&
        export GGCE_ENABLED=1 &&
        export GGCE_DEBUG=1 &&
        export GGCE_VERBOSE=1 &&
        export GGCE_VARIANT=balanced &&
        echo '--- GGCE Environment Check ---' &&
        echo 'GGCE_ENABLED='$GGCE_ENABLED &&
        echo 'GGCE_DEBUG='$GGCE_DEBUG &&
        echo '--- Starting GGCE Test ---' &&
        ./Hierarchy-Clear 3
    "

EXIT_CODE=$?

echo
echo "=== GGCE Test Results ==="
echo "Exit Code: $EXIT_CODE"
if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ GGCE test completed successfully"
elif [ $EXIT_CODE -eq 139 ]; then
    echo "💥 SEGMENTATION FAULT detected!"
else
    echo "❌ Test failed with exit code: $EXIT_CODE"
fi

echo "End time: $(date)"
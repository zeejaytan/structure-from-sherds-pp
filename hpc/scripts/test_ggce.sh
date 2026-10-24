#!/bin/bash

# GGCE Test Script with Segfault Monitoring
# Tests GGCE system functionality and monitors for crashes

echo "=== GGCE System Test ==="
echo "Testing Global Graph Connectivity Enhancement..."

# Set GGCE configuration
export GGCE_ENABLED=1
export GGCE_DEBUG=1
export GGCE_VARIANT=balanced
export GGCE_VERBOSE=1

# Container setup
CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"
DATA_BIND="/data/gpfs/projects/punim2657/sfs_preprocessing/latest_nurbs_dataset/SfS_pp:/Dataset/SfS_pp"

echo "Environment configured:"
echo "  GGCE_ENABLED=$GGCE_ENABLED"
echo "  GGCE_DEBUG=$GGCE_DEBUG"
echo "  GGCE_VARIANT=$GGCE_VARIANT"
echo ""

# Test with small dataset (3 pieces) to watch for segfaults
echo "Running GGCE test with 3 pieces..."
echo "Monitoring for segmentation faults and GGCE debug output..."
echo ""

# Use timeout to prevent hanging, capture both stdout and stderr
timeout 60 $CONTAINER_PATH exec \
    --bind $(pwd):/workspace \
    --bind "$DATA_BIND" \
    /data/gpfs/projects/punim2657/sfs_main/sfspreproc.sif \
    bash -c "
        cd /workspace &&
        export GGCE_ENABLED=1 &&
        export GGCE_DEBUG=1 &&
        export GGCE_VARIANT=balanced &&
        echo '--- Starting GGCE Enhanced Hierarchy-Clear ---' &&
        ./Hierarchy-Clear 3
    " 2>&1

EXIT_CODE=$?

echo ""
echo "=== Test Results ==="
if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ GGCE test completed successfully"
elif [ $EXIT_CODE -eq 124 ]; then
    echo "⏰ Test timed out after 60 seconds (may indicate hanging)"
elif [ $EXIT_CODE -eq 139 ]; then
    echo "💥 SEGMENTATION FAULT detected!"
    echo "   Exit code 139 indicates a segfault occurred"
else
    echo "❌ Test failed with exit code: $EXIT_CODE"
fi

echo "Exit code: $EXIT_CODE"
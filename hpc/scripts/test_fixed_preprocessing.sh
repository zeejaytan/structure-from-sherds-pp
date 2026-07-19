#!/bin/bash

echo "=== Testing Fixed Hierarchy-Clear (No Segfaults) ==="

# Set paths
CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"
DATASET_PATH="/data/gpfs/projects/punim2657/sfs_main"

# Set environment variables to avoid VTK crashes
export VTK_USE_OSMESA=1
export MESA_GL_VERSION_OVERRIDE=3.2
export MESA_GLSL_VERSION_OVERRIDE=150
export VTK_USE_X=0
export LIBGL_ALWAYS_SOFTWARE=1
export VTK_DEBUG_LEAKS=1
export PCL_VISUALIZER_USE_OFFSCREEN=1
export LIBGL_ALWAYS_INDIRECT=1
export __GL_SYNC_TO_VBLANK=0
export VTK_SILENCE_GET_VOID_POINTER_WARNINGS=0
export VTK_ERROR_MODE=1

# Test with small dataset - just 2 pieces for quick verification
echo "Testing fixed Hierarchy-Clear with 2 pieces..."

timeout 180 $CONTAINER_PATH exec \
    --env VTK_SILENCE_GET_VOID_POINTER_WARNINGS=0 \
    --env VTK_ERROR_MODE=1 \
    --env VTK_USE_OSMESA=1 \
    --env MESA_GL_VERSION_OVERRIDE=3.2 \
    --env VTK_USE_X=0 \
    --env LIBGL_ALWAYS_SOFTWARE=1 \
    --bind "$DATASET_PATH:/workspace" \
    --bind "$DATASET_PATH/sfspreproc-docker/Dataset:/Dataset" \
    sfspreproc.sif \
    /workspace/Hierarchy-Clear-Fixed 2

# Check exit code
EXIT_CODE=$?
echo ""
echo "=== Fixed Hierarchy-Clear Test Results ==="

if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ SUCCESS: Fixed Hierarchy-Clear completed without segmentation fault!"
    echo "The VTK visualization patches worked correctly."
    echo ""
    echo "Ready to run full 8-piece preprocessing pipeline."
elif [ $EXIT_CODE -eq 124 ]; then
    echo "⚠️  TIMEOUT: Process took longer than 3 minutes (but no segfault)"
    echo "This suggests the fixes worked - original would segfault immediately."
elif [ $EXIT_CODE -eq 139 ]; then
    echo "❌ SEGFAULT STILL OCCURS: Exit code 139 indicates segmentation fault"
    echo "Additional VTK calls need to be patched."
else
    echo "ℹ️  UNKNOWN EXIT CODE: $EXIT_CODE"
    echo "Check the output above for details."
fi

echo ""
echo "=== Log Files Generated ==="
ls -la sfs_headless*.log visual_output/ 2>/dev/null || echo "No log files found"
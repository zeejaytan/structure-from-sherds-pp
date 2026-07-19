#!/bin/bash

echo "=== AUTOMATED SFS FRAGMENT ASSEMBLY ATTEMPT ==="
echo "Start time: $(date)"
echo

cd /data/gpfs/projects/punim2657/sfs_main

# Set comprehensive environment variables for offscreen/software rendering
export VTK_USE_OSMESA=1
export MESA_GL_VERSION_OVERRIDE=3.2  
export MESA_GLSL_VERSION_OVERRIDE=150
export LIBGL_ALWAYS_SOFTWARE=1
export VTK_USE_X=0
export DISPLAY=:99.0

# Start virtual framebuffer
echo "Starting virtual display..."
Xvfb :99 -screen 0 1024x768x24 -ac > /dev/null 2>&1 &
XVFB_PID=$!
sleep 3

echo "Running SFS with automated input..."

# Try to automate spacebar presses by sending input programmatically
{
    sleep 10   # Wait for initialization
    for i in {1..20}; do
        echo " "   # Send spacebar
        sleep 5    # Wait between steps
    done
    echo "q"       # Try to quit
} | timeout 300 /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --env VTK_USE_OSMESA=1 \
    --env MESA_GL_VERSION_OVERRIDE=3.2 \
    --env LIBGL_ALWAYS_SOFTWARE=1 \
    --env VTK_USE_X=0 \
    --env DISPLAY=:99.0 \
    --bind /data/gpfs/projects/punim2657/sfs_main:/workspace \
    --bind /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset:/Dataset \
    sfspreproc.sif /opt/sfs/bin/Hierarchy-Clear 8

RESULT=$?

# Cleanup
kill $XVFB_PID 2>/dev/null

echo "End time: $(date)"
echo "Exit code: $RESULT"

# Check for results
echo "=== CHECKING FOR ASSEMBLY RESULTS ==="
find /data/gpfs/projects/punim2657/sfs_main -name "*result*" -newer /tmp -o -name "*assembly*" -newer /tmp -o -name "*reconstruction*" -newer /tmp 2>/dev/null | head -10
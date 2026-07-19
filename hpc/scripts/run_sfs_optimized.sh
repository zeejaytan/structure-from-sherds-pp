#!/bin/bash

echo "=== Optimized SfS Run with Multiple Graphics Fallbacks ==="

# Try different OpenGL/VTK configurations
echo "Attempting SfS with graphics optimizations..."

# Method 1: Software rendering fallback
echo ""
echo "=== Attempt 1: Software Rendering ==="
apptainer exec --bind $(pwd):/workspace --bind $(pwd)/sfspreproc-docker/Dataset:/Dataset sfspreproc.sif /bin/bash -c "
    export DISPLAY=$DISPLAY
    export LIBGL_ALWAYS_SOFTWARE=1
    export MESA_GL_VERSION_OVERRIDE=3.3
    export VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN=0
    cd /workspace
    echo 'Trying with software OpenGL rendering...'
    timeout 60 /opt/sfs/bin/Hierarchy-Clear 8 2>&1 | tee /tmp/sfs_attempt1.log
    echo 'Attempt 1 exit code:' $?
" || echo "Attempt 1 failed"

echo ""
echo "=== Attempt 2: Conservative Graphics Settings ==="
apptainer exec --bind $(pwd):/workspace --bind $(pwd)/sfspreproc-docker/Dataset:/Dataset sfspreproc.sif /bin/bash -c "
    export DISPLAY=$DISPLAY
    export LIBGL_ALWAYS_SOFTWARE=1
    export MESA_GL_VERSION_OVERRIDE=2.1
    export VTK_SILENCE_GET_VOID_POINTER_WARNINGS=1
    cd /workspace
    echo 'Trying with conservative OpenGL 2.1...'
    timeout 60 /opt/sfs/bin/Hierarchy-Clear 8 2>&1 | tee /tmp/sfs_attempt2.log
    echo 'Attempt 2 exit code:' $?
" || echo "Attempt 2 failed"

echo ""
echo "=== Attempt 3: Minimal Graphics Mode ==="
apptainer exec --bind $(pwd):/workspace --bind $(pwd)/sfspreproc-docker/Dataset:/Dataset sfspreproc.sif /bin/bash -c "
    export DISPLAY=$DISPLAY
    export LIBGL_ALWAYS_SOFTWARE=1
    export GALLIUM_DRIVER=softpipe
    export MESA_GL_VERSION_OVERRIDE=2.1
    export VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN=0
    export MESA_GLSL_VERSION_OVERRIDE=120
    cd /workspace
    echo 'Trying with minimal graphics mode...'
    timeout 90 /opt/sfs/bin/Hierarchy-Clear 8 2>&1 | tee /tmp/sfs_attempt3.log
    echo 'Attempt 3 exit code:' $?
" || echo "Attempt 3 failed"

echo ""
echo "=== Results Summary ==="
echo "Check the logs to see which attempt got furthest:"
echo "tail /tmp/sfs_attempt*.log"
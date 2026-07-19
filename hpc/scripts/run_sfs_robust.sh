#!/bin/bash

echo "=== Robust SfS Test with Multiple Fallback Methods ==="

# Load required modules
module load Apptainer/1.3.3

# Create results directory
RESULT_DIR="sfs_results_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$RESULT_DIR"
echo "Results will be saved to: $RESULT_DIR"

# Method 1: Try with existing X11 connection (if user has MobaXterm running)
echo ""
echo "=== Method 1: Testing X11 Connection ==="
if xset q &>/dev/null; then
    echo "✓ X11 connection detected! Attempting SfS with visualization..."
    
    apptainer exec --bind $(pwd):/workspace --bind $(pwd)/sfspreproc-docker/Dataset:/Dataset sfspreproc.sif /bin/bash -c "
        export DISPLAY=$DISPLAY
        export LIBGL_ALWAYS_SOFTWARE=1
        export MESA_GL_VERSION_OVERRIDE=2.1
        cd /workspace
        echo 'Running SfS with X11 visualization...'
        timeout 120 /opt/sfs/bin/Hierarchy-Clear 8 > $RESULT_DIR/sfs_x11_log.txt 2>&1
        echo 'X11 attempt exit code:' \$?
    "
    
    echo "X11 attempt log (last 10 lines):"
    tail -10 "$RESULT_DIR/sfs_x11_log.txt" 2>/dev/null || echo "No log file created"
    
else
    echo "✗ No X11 connection available"
    echo "To use visualization:"
    echo "1. Open MobaXterm on your Windows laptop"
    echo "2. Start a new SSH session with X11 forwarding enabled"
    echo "3. Connect to this HPC system and run this script again"
fi

# Method 2: VNC with virtual display (always works)
echo ""
echo "=== Method 2: Virtual Display with Data Processing ==="
echo "This will process all data but skip visualization..."

apptainer exec --bind $(pwd):/workspace --bind $(pwd)/sfspreproc-docker/Dataset:/Dataset sfspreproc.sif /bin/bash -c "
    # Start virtual display
    export DISPLAY=:99
    Xvfb :99 -screen 0 1280x720x24 -ac &
    XVFB_PID=\$!
    sleep 2
    
    cd /workspace
    echo 'Starting SfS with virtual display...'
    echo 'This will complete data processing but crash at visualization'
    
    # Run SfS - expect it to crash at VTK but process data first
    timeout 60 /opt/sfs/bin/Hierarchy-Clear 8 > $RESULT_DIR/sfs_virtual_log.txt 2>&1
    EXIT_CODE=\$?
    
    echo \"Virtual display attempt exit code: \$EXIT_CODE\"
    
    # Kill virtual display
    kill \$XVFB_PID 2>/dev/null
    
    # Show what was accomplished
    echo 'Data processing results:'
    grep -E '(Successfully|points|Ground Truth|Pottery Data|Axis)' $RESULT_DIR/sfs_virtual_log.txt | tail -20
"

echo ""
echo "=== Results Summary ==="
echo "Check logs in: $RESULT_DIR/"
ls -la "$RESULT_DIR/"

echo ""
echo "=== Analysis ==="
echo "Virtual display log (key sections):"
if [ -f "$RESULT_DIR/sfs_virtual_log.txt" ]; then
    echo "--- Data Loading Results ---"
    grep -A5 "Pottery Data load" "$RESULT_DIR/sfs_virtual_log.txt" || echo "Data loading section not found"
    
    echo ""
    echo "--- Processing Status ---"
    grep -E "(Successfully|Error|points loaded)" "$RESULT_DIR/sfs_virtual_log.txt" | tail -10
    
    echo ""
    echo "--- Final Status ---"
    tail -5 "$RESULT_DIR/sfs_virtual_log.txt"
else
    echo "No virtual display log found"
fi

echo ""
echo "=== Next Steps ==="
echo "1. If X11 worked: The visualization ran successfully on your laptop"
echo "2. If only virtual display worked: Data was processed but needs visualization fix"
echo "3. Check the logs above to see exactly what was accomplished"
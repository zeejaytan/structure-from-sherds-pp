#!/bin/bash

echo "=== SfS Auto-Save Approach ==="
echo "This will run SfS and attempt to trigger save operations before it crashes"

# Create output directory
mkdir -p sfs_results_$(date +%Y%m%d_%H%M%S)
RESULT_DIR="sfs_results_$(date +%Y%m%d_%H%M%S)"

echo "Results will be saved to: $RESULT_DIR"

# Run SfS with aggressive auto-save attempts
apptainer exec --bind $(pwd):/workspace --bind $(pwd)/sfspreproc-docker/Dataset:/Dataset sfspreproc.sif /bin/bash -c "
    export DISPLAY=$DISPLAY
    export LIBGL_ALWAYS_SOFTWARE=1
    export MESA_GL_VERSION_OVERRIDE=2.1
    cd /workspace
    
    echo 'Starting SfS with auto-save monitoring...'
    echo 'Will attempt to save results when processing completes...'
    
    # Run SfS in background
    /opt/sfs/bin/Hierarchy-Clear 8 > $RESULT_DIR/sfs_full_log.txt 2>&1 &
    SFS_PID=\$!
    
    echo \"SfS started with PID: \$SFS_PID\"
    
    # Monitor the process and try to send save commands at the right time
    sleep 8  # Wait for data loading to complete
    
    if kill -0 \$SFS_PID 2>/dev/null; then
        echo 'Sending spacebar to show first result...'
        # Try multiple methods to send spacebar
        echo -e ' \n' > /proc/\$SFS_PID/fd/0 2>/dev/null || echo 'STDIN method failed'
        
        sleep 2
        
        if kill -0 \$SFS_PID 2>/dev/null; then
            echo 'Sending save command (s key)...'
            echo -e 's\n' > /proc/\$SFS_PID/fd/0 2>/dev/null || echo 'Save STDIN method failed'
            
            sleep 2
            
            if kill -0 \$SFS_PID 2>/dev/null; then
                echo 'Sending transformation save (g key)...'
                echo -e 'g\n' > /proc/\$SFS_PID/fd/0 2>/dev/null || echo 'Transform STDIN method failed'
                
                sleep 2
            fi
        fi
    fi
    
    # Wait for process to complete or crash
    wait \$SFS_PID 2>/dev/null
    EXIT_CODE=\$?
    
    echo \"SfS process completed with exit code: \$EXIT_CODE\"
    
    # Check what was accomplished
    echo 'Checking for any generated results...'
    find /Dataset -name '*Result*' -type d 2>/dev/null || echo 'No Result directories found'
    find /workspace -name '*.obj' -newer $RESULT_DIR 2>/dev/null | head -5
    
    # Copy any results that might have been created
    cp -r /Dataset/Result/* $RESULT_DIR/ 2>/dev/null || echo 'No results in /Dataset/Result/'
    
    echo 'Log file information:'
    wc -l $RESULT_DIR/sfs_full_log.txt 2>/dev/null || echo 'No log file'
    echo 'Last 20 lines of processing:'
    tail -20 $RESULT_DIR/sfs_full_log.txt 2>/dev/null || echo 'Cannot read log'
"

echo ""
echo "=== Auto-Save Attempt Complete ==="
echo "Check results in: $RESULT_DIR/"
ls -la "$RESULT_DIR/" 2>/dev/null || echo "Results directory not created"
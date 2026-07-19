#!/bin/bash

# SfS X11 Forwarding Test Script
# Run this after SSH with X11 forwarding: ssh -X user@spartan.hpc.unimelb.edu.au

echo "=== Testing X11 Forwarding for SfS Visualization ==="
echo "Current DISPLAY: $DISPLAY"

# Test basic X11 forwarding
echo "Testing basic X11 forwarding..."
which xeyes && timeout 5 xeyes &
which xclock && timeout 5 xclock &
sleep 2
pkill xeyes xclock 2>/dev/null

echo ""
echo "Loading required modules..."
module load Apptainer/1.3.3

echo ""
echo "Setting up data paths..."
cd /data/gpfs/projects/punim2657/sfspre

echo ""
echo "Testing SfS with X11 forwarding to your laptop..."
echo "This should open the SfS visualization window on your laptop!"
echo ""
echo "Controls once it opens:"
echo "  - Spacebar: Show first reconstruction result"  
echo "  - Left/Right arrows: Navigate between results"
echo "  - 's': Save current result"
echo "  - 'g': Save transformation matrices"
echo "  - 'f': Compute accuracy"
echo ""

# Run SfS with X11 forwarding
apptainer exec --bind $(pwd):/workspace --bind $(pwd)/sfspreproc-docker/Dataset:/Dataset sfspreproc.sif \
/bin/bash -c "
    export DISPLAY=$DISPLAY
    cd /workspace
    echo 'Starting SfS with visualization forwarded to your laptop...'
    /opt/sfs/bin/Hierarchy-Clear 8
"

echo ""
echo "SfS X11 forwarding test complete!"
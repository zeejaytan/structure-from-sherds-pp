#!/bin/bash

# VNC Setup for SfS Visualization Access
# Alternative to X11 forwarding - more stable for complex graphics

echo "=== Setting up VNC Server for SfS Remote Visualization ==="

# Check if VNC is available
if ! command -v vncserver &> /dev/null; then
    echo "VNC server not available on this system"
    echo "Please use X11 forwarding instead: ssh -X user@spartan.hpc.unimelb.edu.au"
    exit 1
fi

# Start VNC server
echo "Starting VNC server..."
vncserver :1 -geometry 1280x720 -depth 24

echo ""
echo "VNC server started on display :1"
echo ""
echo "To connect from your laptop:"
echo "1. Create SSH tunnel: ssh -L 5901:localhost:5901 user@spartan.hpc.unimelb.edu.au"
echo "2. Open VNC viewer and connect to: localhost:5901"
echo "3. Enter your VNC password when prompted"
echo ""

# Set up environment for VNC session
export DISPLAY=:1

echo "Setting up SfS environment in VNC session..."
cd /data/gpfs/projects/punim2657/sfspre

# Load modules
module load Apptainer/1.3.3

echo ""
echo "To run SfS in the VNC session, execute:"
echo "apptainer exec --bind \$(pwd):/workspace --bind \$(pwd)/sfspreproc-docker/Dataset:/Dataset sfspreproc.sif /opt/sfs/bin/Hierarchy-Clear 8"
echo ""
echo "VNC setup complete! Connect from your laptop to see the visualization."
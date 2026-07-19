#!/bin/bash

# Run SFS Test with Pot_A data
echo "=== Running SFS Reconstruction Test ==="

# Load modules
module load Apptainer/1.3.3

# Configuration  
CONTAINER="sfspreproc.sif"
OUTPUT_DIR="sfs_test_reconstruction"
DATA_PATH="/Dataset/SfS_pp"

echo "Setting up SFS reconstruction test..."

# Create output directory
mkdir -p "$OUTPUT_DIR"
cd "$OUTPUT_DIR"

# Copy data for test (first few pieces of Pot_A)
echo "Setting up test data structure..."
mkdir -p Dataset/SfS_pp/{Breaklines,Axes,Surfaces,Mesh}

# Copy just a few pieces for testing
for piece in 01 02 03; do
    cp "../sfspreproc-docker/Dataset/SfS_pp/Breaklines/Pot_A_Piece_${piece}_Breakline_0.pcd" Dataset/SfS_pp/Breaklines/ 2>/dev/null
    cp "../sfspreproc-docker/Dataset/SfS_pp/Breaklines/Pot_A_Piece_${piece}_Breakline_1.pcd" Dataset/SfS_pp/Breaklines/ 2>/dev/null  
    cp "../sfspreproc-docker/Dataset/SfS_pp/Axes/Pot_A_Piece_${piece}_Axis.xyz" Dataset/SfS_pp/Axes/ 2>/dev/null
    cp "../sfspreproc-docker/Dataset/SfS_pp/Mesh/Pot_A_Piece_${piece}_Mesh.obj" Dataset/SfS_pp/Mesh/ 2>/dev/null
done

echo "Test data prepared:"
ls -la Dataset/SfS_pp/*/

# Test the main SFS processing
echo "Running SFS reconstruction..."

# Try to run the hierarchy clear tool first
echo "1. Testing Hierarchy-Clear tool:"
apptainer exec --bind $(pwd):/work "../$CONTAINER" /bin/bash -c "
    cd /work
    echo 'Current directory contents:'
    ls -la
    echo 'Running Hierarchy-Clear:'
    /opt/sfs/bin/Hierarchy-Clear
" 2>&1 | head -20

echo "✓ SFS reconstruction test completed"
echo "Check the output directory for results"
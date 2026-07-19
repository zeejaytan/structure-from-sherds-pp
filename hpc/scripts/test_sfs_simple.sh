#!/bin/bash

# Simple SFS Main System Test
echo "=== SFS Main System Test ==="

# Load modules
module load Apptainer/1.3.3

# Configuration
CONTAINER="sfspreproc.sif"
DATA_DIR="sfspreproc-docker/Dataset/SfS_pp"
OUTPUT_DIR="test_sfs_output"

echo "Testing main SFS system functionality..."

# Create output directory
mkdir -p "$OUTPUT_DIR"
cd "$OUTPUT_DIR"

# Test 1: Check container and data
echo "1. Checking container and data availability..."

if [ -f "../$CONTAINER" ]; then
    echo "✓ Container found: $CONTAINER"
else
    echo "✗ Container not found: $CONTAINER"
    exit 1
fi

if [ -d "../$DATA_DIR" ]; then
    echo "✓ Data directory found: $DATA_DIR"
    echo "Available data:"
    ls -1 ../$DATA_DIR/Breaklines/ | grep "Pot_A" | head -5
else
    echo "✗ Data directory not found: $DATA_DIR"
    exit 1
fi

# Test 2: Check SFS executables
echo "2. Testing SFS executables..."
apptainer exec "../$CONTAINER" /bin/bash -c "
    echo 'Available SFS tools:'
    ls -la /opt/sfs/bin/
    echo 'Testing help for main tools:'
    /opt/sfs/bin/preprocess-mesh --help 2>/dev/null || echo 'No help available for preprocess-mesh'
    /opt/sfs/bin/extract-edges --help 2>/dev/null || echo 'No help available for extract-edges'
"

# Test 3: Basic data processing test
echo "3. Basic data inspection..."

# Copy a small subset of data
mkdir -p test_data/{Breaklines,Axes,Surfaces}
cp ../$DATA_DIR/Breaklines/Pot_A_Piece_01_Breakline_0.pcd test_data/Breaklines/ 2>/dev/null
cp ../$DATA_DIR/Axes/Pot_A_Piece_01_Axis.xyz test_data/Axes/ 2>/dev/null

echo "Test data copied:"
ls -la test_data/*/

# Test data format
echo "4. Checking data formats..."
echo "Axis file content:"
head -5 test_data/Axes/Pot_A_Piece_01_Axis.xyz 2>/dev/null || echo "Axis file not found"

echo "Breakline file info:"
apptainer exec --bind $(pwd)/test_data:/data "../$CONTAINER" /bin/bash -c "
    file /data/Breaklines/*.pcd
    echo 'PCD file header:'
    head -10 /data/Breaklines/*.pcd | grep -E '^(VERSION|FIELDS|SIZE|TYPE|COUNT|WIDTH|HEIGHT|POINTS)'
"

echo "✓ Simple SFS test completed!"
echo "Next steps: Run full reconstruction with multiple fragments"
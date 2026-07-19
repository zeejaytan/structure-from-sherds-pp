#!/bin/bash

# Build script for fixed Hierarchy-Clear without segfaults

echo "=== Building Fixed Hierarchy-Clear (No Segfaults) ==="

# Set container path
CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"

# Copy the fixed source files into the container workspace
echo "Copying fixed source files..."
cp -r /data/gpfs/projects/punim2657/sfs_main/sfs_modified_src/* /data/gpfs/projects/punim2657/sfs_main/container_build_space/

# Build inside container with all dependencies
echo "Building inside container..."
$CONTAINER_PATH exec \
    --bind /data/gpfs/projects/punim2657/sfs_main:/workspace \
    --bind /data/gpfs/projects/punim2657/sfs_main/container_build_space:/build_src \
    sfspreproc.sif \
    /bin/bash -c "
        cd /build_src && 
        rm -f *.o Hierarchy-Clear main_headless_correct &&
        cmake . -DCMAKE_BUILD_TYPE=Release &&
        make -j4 &&
        cp main_headless_correct /workspace/Hierarchy-Clear-Fixed &&
        echo 'Build completed! Fixed binary: /workspace/Hierarchy-Clear-Fixed'
    "

if [ $? -eq 0 ]; then
    echo "=== Fixed Hierarchy-Clear build successful ==="
    ls -la /data/gpfs/projects/punim2657/sfs_main/Hierarchy-Clear-Fixed
    echo "Ready for segfault-free preprocessing!"
else
    echo "=== Build failed ==="
    exit 1
fi
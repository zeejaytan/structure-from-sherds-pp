#!/bin/bash

echo "=== Building TPS Normals Computer in Container ==="

CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer"

# Build inside container where Eigen3 is available
$CONTAINER_PATH exec \
    --bind /data/gpfs/projects/punim2657/sfs_main:/workspace \
    sfspreproc.sif \
    /bin/bash -c "
        cd /workspace && \
        g++ -std=c++11 -O2 -I/usr/include/eigen3 \
            compute_tps_normals.cpp \
            -o compute_tps_normals && \
        echo '✅ TPS normals computer built successfully' && \
        ls -la compute_tps_normals
    "

if [ $? -eq 0 ]; then
    echo "✅ Build complete - ready to compute TPS normals"
else
    echo "❌ Container build failed"
    exit 1
fi
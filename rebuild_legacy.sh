#!/bin/bash
# Rebuild legacy SFS++ with relaxed thresholds

CONTAINER_PATH="/apps/easybuild-2022/easybuild/software/Core/Apptainer/1.3.3/bin/apptainer"
SIF_FILE="/data/gpfs/projects/punim2657/sfs_preprocessing/pcl_191_nurbs.sif"

echo "=== Cleaning build directory ==="
cd /data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp/structure-from-sherds-pp/build
rm -rf *

echo "=== Building inside container ==="
$CONTAINER_PATH exec --bind /data/gpfs/projects/punim2657:/data/gpfs/projects/punim2657 \
    $SIF_FILE /bin/bash -c \
    "cd /data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp/structure-from-sherds-pp/build && \
     cmake .. && \
     make -j8"

echo "=== Build complete ==="
ls -lh /data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp/structure-from-sherds-pp/build/ReconstructionHeadless

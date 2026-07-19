#!/bin/bash
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=12:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=4
#SBATCH --mem=16G
#SBATCH --job-name=rebuild_top_k

# SIMPLE REBUILD APPROACH - JUST BUILD INSIDE CONTAINER PROPERLY

echo "=== REBUILDING TOP-K ENHANCED BINARY ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Strategy: Build inside container using container's native compilers"

cd /data/gpfs/projects/punim2657/sfs_main

echo ""
echo "Step 1: Clean previous build artifacts..."
/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    sfspreproc.sif \
    bash -c "cd /workspace/sfspreproc-docker && rm -rf CMakeFiles CMakeCache.txt cmake_install.cmake Hierarchy-Clear"

echo ""
echo "Step 2: Build with container's native build environment..."
echo "Using container's gcc/cmake instead of host system paths"

/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    sfspreproc.sif \
    bash -c "cd /workspace/sfspreproc-docker &&
             export CC=/usr/bin/gcc &&
             export CXX=/usr/bin/g++ &&
             cmake -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ . &&
             make clean &&
             make -j 4"

if [ $? -ne 0 ]; then
    echo "❌ Build failed - checking what went wrong..."

    # Show container compiler info
    echo "Container compiler info:"
    /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
        sfspreproc.sif \
        bash -c "which gcc && gcc --version"

    exit 1
fi

echo ""
echo "✅ Build completed successfully!"

# Verify TOP-K features are compiled
echo ""
echo "Step 3: Verify TOP-K features in binary..."
echo "Checking source for TOP-K features:"
grep -q "ENABLE_TOP_K_TRANSFORMS" sfspreproc-docker/main_headless_correct.cpp && echo "✅ TOP-K source found" || echo "❌ No TOP-K source"

echo "Binary info:"
echo "Size: $(ls -lh sfspreproc-docker/Hierarchy-Clear | awk '{print $5}')"
echo "Built: $(stat -c %y sfspreproc-docker/Hierarchy-Clear)"

echo ""
echo "Step 4: Test run with TOP-K enabled binary..."

time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    /workspace/sfspreproc-docker/Hierarchy-Clear 8

echo ""
echo "=== TOP-K REBUILD AND TEST RESULTS ==="
echo "Completed at: $(date)"

# Analyze results
LATEST_RESULTS=$(find /data/gpfs/projects/punim2657/sfs_main -name "results_*" -type d | sort | tail -1)

if [ -n "$LATEST_RESULTS" ] && [ -d "$LATEST_RESULTS" ]; then
    echo "✅ Results: $(basename $LATEST_RESULTS)"

    if [ -f "$LATEST_RESULTS/assembly_data.txt" ]; then
        echo ""
        echo "=== TOP-K TRANSFORMATION ANALYSIS ==="
        unique_graphs=$(grep "Transformation found in graph" "$LATEST_RESULTS/assembly_data.txt" | sed 's/.*graph //' | sort | uniq | wc -l)
        echo "Graphs used: $unique_graphs"

        if [ "$unique_graphs" -eq 1 ]; then
            echo "🎯 BREAKTHROUGH! UNIFIED GRAPH ACHIEVED!"
            echo "TOP-K transformations successfully resolved spatial constraints!"
        elif [ "$unique_graphs" -eq 2 ]; then
            echo "⚡ MAJOR IMPROVEMENT! Reduced from 3 to $unique_graphs graphs!"
            echo "TOP-K transformations are working!"
        else
            echo "📊 Current: $unique_graphs graphs (baseline was 3)"
        fi

        # Look for TOP-K debug output
        echo ""
        echo "=== TOP-K DEBUG OUTPUT CHECK ==="
        top_k_events=$(grep -c "*** TOP-K DEBUG ***" slurm-$SLURM_JOB_ID.out 2>/dev/null || echo "0")
        transform_gen=$(grep -c "Generated.*transformation options" slurm-$SLURM_JOB_ID.out 2>/dev/null || echo "0")

        echo "TOP-K debug events: $top_k_events"
        echo "Transformation generations: $transform_gen"

        if [ "$top_k_events" -gt 0 ]; then
            echo "✅ TOP-K transformations are active!"
        else
            echo "⚠ No TOP-K debug output - may need #define check"
        fi
    fi
else
    echo "❌ No results directory found"
fi

echo ""
echo "Rebuild and test completed."
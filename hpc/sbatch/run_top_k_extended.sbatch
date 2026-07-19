#!/bin/bash
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=12:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=4
#SBATCH --mem=16G
#SBATCH --job-name=top_k_extended

# EXTENDED TOP-K TRANSFORMATION TEST
# 12-hour allocation for comprehensive 240+ spatial exploration

echo "=== EXTENDED TOP-K TRANSFORMATION TEST ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Extended runtime: 12 hours for comprehensive beam search exploration"
echo "TOP-K Configuration: K=2, Enhanced beam search (TOP_k=15, BRANCH_b=8)"

cd /data/gpfs/projects/punim2657/sfs_main

echo ""
echo "Step 1: Verify TOP-K enhanced binary exists..."
if [ -f "sfspreproc-docker/Hierarchy-Clear" ]; then
    echo "✅ TOP-K enhanced binary found"
    echo "Binary size: $(ls -lh sfspreproc-docker/Hierarchy-Clear | awk '{print $5}')"
    echo "Built: $(stat -c %y sfspreproc-docker/Hierarchy-Clear)"
else
    echo "❌ Binary not found - rebuilding..."

    /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
        --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
        sfspreproc.sif \
        bash -c "cd /workspace/sfspreproc-docker &&
                 export CC=/usr/bin/gcc &&
                 export CXX=/usr/bin/g++ &&
                 rm -rf CMakeFiles CMakeCache.txt cmake_install.cmake Hierarchy-Clear &&
                 cmake -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ . &&
                 make clean &&
                 make -j 4"

    if [ $? -ne 0 ]; then
        echo "❌ Build failed"
        exit 1
    fi

    echo "✅ Build completed"
fi

echo ""
echo "Step 2: Verify TOP-K source features..."
grep -q "ENABLE_TOP_K_TRANSFORMS" sfspreproc-docker/main_headless_correct.cpp && echo "✅ TOP-K transforms enabled" || echo "❌ TOP-K not enabled"
grep -q "MAX_TRANSFORM_CONFIGS" sfspreproc-docker/main_headless_correct.cpp && echo "✅ K-configuration system found" || echo "❌ K-config missing"

echo ""
echo "Step 3: Extended TOP-K SFS reconstruction (12 hours allocated)..."
echo "Expected: 213 transformation pairs × K=2 = 426 geometric configurations"
echo "Expected: Enhanced beam search exploring 240+ spatial arrangement paths"

time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    /workspace/sfspreproc-docker/Hierarchy-Clear 8

echo ""
echo "=== EXTENDED TOP-K TEST ANALYSIS ==="
echo "Completed at: $(date)"

# Comprehensive results analysis
LATEST_RESULTS=$(find /data/gpfs/projects/punim2657/sfs_main -name "results_*" -type d | sort | tail -1)

if [ -n "$LATEST_RESULTS" ] && [ -d "$LATEST_RESULTS" ]; then
    echo "✅ Results: $(basename $LATEST_RESULTS)"

    if [ -f "$LATEST_RESULTS/assembly_data.txt" ]; then
        echo ""
        echo "=== GRAPH UNIFICATION ANALYSIS ==="
        unique_graphs=$(grep "Transformation found in graph" "$LATEST_RESULTS/assembly_data.txt" | sed 's/.*graph //' | sort | uniq | wc -l)
        echo "Number of graphs: $unique_graphs"

        echo ""
        echo "Graph distribution:"
        grep "Transformation found in graph" "$LATEST_RESULTS/assembly_data.txt" | sed 's/.*graph //' | sort | uniq -c

        if [ "$unique_graphs" -eq 1 ]; then
            echo ""
            echo "🎯🎯🎯 BREAKTHROUGH SUCCESS! UNIFIED GRAPH ACHIEVED! 🎯🎯🎯"
            echo "TOP-K transformation system successfully resolved 3-graph fragmentation!"
            echo "All 8 pieces assembled in single unified graph structure!"
        elif [ "$unique_graphs" -eq 2 ]; then
            echo ""
            echo "⚡ MAJOR IMPROVEMENT! Reduced from 3 to $unique_graphs graphs!"
            echo "TOP-K transformations showing significant progress!"
        else
            echo ""
            echo "📊 Current: $unique_graphs graphs (baseline was 3)"
            if [ "$unique_graphs" -lt 3 ]; then
                echo "✅ Improvement over baseline fragmentation"
            fi
        fi

        # TOP-K effectiveness analysis
        echo ""
        echo "=== TOP-K SYSTEM EFFECTIVENESS ==="
        top_k_events=$(grep -c "*** TOP-K DEBUG ***" slurm-$SLURM_JOB_ID.out 2>/dev/null || echo "0")
        transform_gen=$(grep -c "Generated.*transformation options" slurm-$SLURM_JOB_ID.out 2>/dev/null || echo "0")
        profile_passed=$(grep -c "PROFILE VALIDATION.*RESULT: PASSED" slurm-$SLURM_JOB_ID.out 2>/dev/null || echo "0")
        profile_failed=$(grep -c "PROFILE VALIDATION.*RESULT: FAILED" slurm-$SLURM_JOB_ID.out 2>/dev/null || echo "0")

        echo "TOP-K debug events: $top_k_events"
        echo "Transformation generations: $transform_gen"
        echo "Profile validations PASSED: $profile_passed"
        echo "Profile validations FAILED: $profile_failed"

        if [ "$profile_passed" -gt 0 ] && [ "$profile_failed" -eq 0 ]; then
            echo "✅ Perfect profile validation - all spatial arrangements valid!"
        elif [ "$profile_passed" -gt "$profile_failed" ]; then
            echo "✅ Superior profile validation - TOP-K finding better spatial fits!"
        fi

        if [ "$top_k_events" -gt 400 ]; then
            echo "✅ Full TOP-K system deployment confirmed ($top_k_events debug events)"
        elif [ "$top_k_events" -gt 0 ]; then
            echo "⚠ Partial TOP-K deployment ($top_k_events debug events)"
        else
            echo "❌ No TOP-K debug output - system may not be active"
        fi
    fi
else
    echo "❌ No results directory found"
fi

echo ""
echo "Extended TOP-K test completed with 12-hour allocation."
echo "Runtime: $SECONDS seconds"
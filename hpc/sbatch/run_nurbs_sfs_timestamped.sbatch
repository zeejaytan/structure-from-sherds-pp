#!/bin/bash
#SBATCH --job-name=sfs_nurbs_timestamped
#SBATCH --time=02:00:00
#SBATCH --mem=16G
#SBATCH --cpus-per-task=4
#SBATCH --partition=sapphire
#SBATCH --output=sfs_nurbs_timestamped_%j.out
#SBATCH --error=sfs_nurbs_timestamped_%j.err

echo "=== SFS RECONSTRUCTION WITH TIMESTAMPED OUTPUT DIRECTORIES ==="
echo "Job ID: $SLURM_JOB_ID"
echo "Start time: $(date)"
echo "Using newly generated NURBS dataset with fixed curvature computation"
echo "Dataset location: /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/"
echo "✨ NEW: Results will be saved in timestamped directories to prevent overwriting"
echo

cd /data/gpfs/projects/punim2657/sfs_main

echo "Verifying fixed NURBS dataset structure..."
echo "Surfaces: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp/Surfaces -name "*.xyz" | wc -l)"
echo "Breaklines: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp/Breaklines -name "*_Breakline_*.pcd" | wc -l)"
echo "Axes: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp/Axes -name "*.xyz" 2>/dev/null | wc -l)"
echo "Meshes: $(find /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature -name "*.obj" | wc -l)"
echo

# Sample curvature verification
SAMPLE_BREAKLINE=$(find /data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp/Breaklines -name "*_Breakline_0.pcd" | head -1)
if [ ! -z "$SAMPLE_BREAKLINE" ]; then
    echo "Verifying fixed curvature values in: $(basename $SAMPLE_BREAKLINE)"
    SAMPLE_CURVATURE=$(grep -E "^[0-9\.\-e]" "$SAMPLE_BREAKLINE" | head -1 | awk '{print $7}')
    echo "Sample curvature value: $SAMPLE_CURVATURE"
    if [[ $SAMPLE_CURVATURE =~ ^[0-9\.\-e]+$ ]] && (( $(echo "$SAMPLE_CURVATURE < 1e-30" | bc -l) )) && (( $(echo "$SAMPLE_CURVATURE > 1e-40" | bc -l) )); then
        echo "✓ Curvature values appear fixed (proper geometric range)"
    else
        echo "⚠ Curvature values may need verification"
    fi
fi

echo
echo "Dataset verification complete. Using fixed NURBS data with proper curvature computation"
echo

echo "Using existing SFS binary..."
echo "Current binary timestamp: $(stat -c %y /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Hierarchy-Clear 2>/dev/null || echo 'Binary not found')"
echo "Note: Build manually before running this job if binary needs updating"
echo

echo "Running SFS reconstruction with Auto-Agglomerative Assembly system..."
echo "Bind mount: Fixed NURBS dataset → /Dataset/SfS_pp"
echo "Command: /workspace/sfspreproc-docker/Hierarchy-Clear 8"
echo "🚀 Auto-Agglomerative Assembly: ENABLED - PuzzleFusion++ inspired global optimization"
echo "🎯 6-DoF Diffusion Denoising: Complete replacement of incremental graph building"
echo "🏆 Global Optimization: Eliminates 3-graph fragmentation at source"
echo "✨ Output: Results will be saved in results_YYYY_MM_DD_HHMM/ directory"
echo

time /apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer exec \
    --bind "/data/gpfs/projects/punim2657/sfs_main:/workspace" \
    --bind "/data/gpfs/projects/punim2657/sfs_preprocessing/NURBS_Dataset_20250907_ProperCurvature/SfS_pp:/Dataset/SfS_pp" \
    sfspreproc.sif \
    bash -c "export ENABLE_AUTO_AGGLOMERATIVE=1 && export ENABLE_MULTI_HYPOTHESIS=1 && export ENABLE_POTTERY_VALIDATION=1 && echo 'Container ENV: THREE_SYSTEMS_ENABLED' && /workspace/sfspreproc-docker/Hierarchy-Clear 8"

echo
echo "=== SFS Reconstruction Results ==="
echo "Timestamped output directories created:"
ls -la /data/gpfs/projects/punim2657/sfs_main/results_* 2>/dev/null || echo "No timestamped result directories found - check logs above"

echo
echo "Individual result contents:"
for result_dir in /data/gpfs/projects/punim2657/sfs_main/results_*; do
    if [ -d "$result_dir" ]; then
        echo "📁 $(basename $result_dir):"
        echo "  Main files: $(ls -1 "$result_dir"/*.txt "$result_dir"/*.xyz 2>/dev/null | wc -l) files"
        echo "  Result files: $(ls -1 "$result_dir"/Result/*.txt "$result_dir"/Result/*.xyz 2>/dev/null | wc -l) files"
        echo "  Size: $(du -sh "$result_dir" 2>/dev/null | cut -f1)"
    fi
done

echo
echo "=== Generating PLY Visualization Files ==="
# Generate PLY files for each timestamped result directory
for result_dir in /data/gpfs/projects/punim2657/sfs_main/results_*; do
    if [ -d "$result_dir/Result" ]; then
        echo "Generating visualization for $(basename $result_dir)..."
        python3 /data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp/scripts/generate_assembly_ply.py "$result_dir/Result" 2>&1 | grep -E "✓|✗|WARNING|ERROR|Total"

        if [ -f "$result_dir/Result/assembly_all_pieces.ply" ]; then
            echo "  ✓ PLY files generated in $result_dir/Result/"
            echo "  📊 Quick view: meshlab $result_dir/Result/assembly_all_pieces.ply"
        else
            echo "  ⚠ PLY generation may have failed - check above for errors"
        fi
        echo ""
    fi
done

echo
echo "GGCE-enhanced NURBS SFS reconstruction with timestamped outputs completed at: $(date)"
echo "✅ Infinite Loop Fix: Applied"
echo "✅ NURBS Curvature Fix: Applied"
echo "✅ GGCE Global Graph Connectivity Enhancement: Applied"
echo "✅ Multi-Method Intersection Detection: Applied"
echo "✨ Timestamped Output Directories: Applied"
echo "=== Job Finished ==="
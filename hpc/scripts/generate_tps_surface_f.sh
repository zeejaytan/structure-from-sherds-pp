#!/bin/bash

echo "=== GENERATING SURFACE_F WITH PROPER TPS NORMALS ==="
echo "Computing mathematically correct normals from TPS derivatives"
echo ""

# Backup old Surface_F files
mkdir -p /tmp/old_surface_f_backup
cp /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_*Surface_F* /tmp/old_surface_f_backup/ 2>/dev/null

for piece in {01..08}; do
    surface_0="/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_Piece_${piece}_Surface_0.xyz"
    surface_1="/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_Piece_${piece}_Surface_1.xyz"
    surface_f_pcd="/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_Piece_${piece}_Surface_F.pcd"
    surface_f_xyz="/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_Piece_${piece}_Surface_F.xyz"
    
    echo "Processing Piece $piece..."
    
    if [[ -f "$surface_0" ]]; then
        echo "  Computing TPS normals from Surface_0..."
        
        # Use the TPS normals computer to generate proper Surface_F with normals
        ./compute_tps_normals "$surface_0" "$surface_f_pcd" "$surface_f_xyz"
        
        if [[ $? -eq 0 ]]; then
            echo "  ✅ Generated Surface_F with TPS normals for piece $piece"
        else
            echo "  ⚠️ TPS normals computation failed, using Surface_1 as fallback..."
            ./compute_tps_normals "$surface_1" "$surface_f_pcd" "$surface_f_xyz"
            
            if [[ $? -eq 0 ]]; then
                echo "  ✅ Generated Surface_F from Surface_1 for piece $piece"
            else
                echo "  ❌ Both Surface_0 and Surface_1 failed for piece $piece"
            fi
        fi
    else
        echo "  ❌ Surface_0 not found for piece $piece"
    fi
    
    echo ""
done

echo "=== VERIFICATION OF TPS SURFACE_F FILES ==="

# Check first file to verify format
first_file="/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_Piece_01_Surface_F.pcd"
if [[ -f "$first_file" ]]; then
    echo "Sample TPS Surface_F format:"
    head -15 "$first_file"
    echo ""
    
    echo "Format verification:"
    grep "FIELDS" "$first_file"
    echo "Points count: $(grep "POINTS" "$first_file" | grep -v "WIDTH")"
    echo ""
    
    # Check if normals look reasonable (should not be all zeros)
    echo "Sample normal vectors:"
    tail -n +12 "$first_file" | head -3 | awk '{print "Normal: [" $4 ", " $5 ", " $6 "]"}'
    echo ""
fi

echo "Generated Surface_F files: $(ls /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_*Surface_F* 2>/dev/null | wc -l)"
echo "✅ TPS Surface_F generation with proper normals complete!"
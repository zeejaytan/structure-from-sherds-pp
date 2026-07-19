#!/bin/bash

echo "=== BACKING UP AND CLEANING OLD PREPROCESSING DATA ==="

# Create timestamped backup directory
BACKUP_DIR="/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp_BACKUP_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$BACKUP_DIR"/{Axes,Breaklines,Surfaces}

echo "Backup directory: $BACKUP_DIR"

# Backup old Pot A preprocessing files
echo "Backing up old Axes files..."
cp /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Axes/Pot_A_* "$BACKUP_DIR/Axes/" 2>/dev/null

echo "Backing up old Breaklines files..."
cp /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Breaklines/Pot_A_* "$BACKUP_DIR/Breaklines/" 2>/dev/null

echo "Backing up old Surface_F files..."
cp /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_*Surface_F* "$BACKUP_DIR/Surfaces/" 2>/dev/null

# Count backed up files
AXES_COUNT=$(ls "$BACKUP_DIR/Axes/" 2>/dev/null | wc -l)
BREAKLINES_COUNT=$(ls "$BACKUP_DIR/Breaklines/" 2>/dev/null | wc -l)
SURFACES_COUNT=$(ls "$BACKUP_DIR/Surfaces/" 2>/dev/null | wc -l)

echo "=== BACKUP SUMMARY ==="
echo "Axes files backed up: $AXES_COUNT"
echo "Breaklines files backed up: $BREAKLINES_COUNT"
echo "Surface_F files backed up: $SURFACES_COUNT"
echo "Total files backed up: $((AXES_COUNT + BREAKLINES_COUNT + SURFACES_COUNT))"

# Now delete the old files to create clean slate
echo ""
echo "=== DELETING OLD PREPROCESSING FILES ==="

echo "Deleting old Axes files..."
rm -f /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Axes/Pot_A_*

echo "Deleting old Breaklines files..."
rm -f /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Breaklines/Pot_A_*

echo "Deleting old Surface_F files..."
rm -f /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_*Surface_F*

echo ""
echo "=== VERIFICATION ==="
echo "Remaining Axes files: $(ls /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Axes/Pot_A_* 2>/dev/null | wc -l)"
echo "Remaining Breaklines files: $(ls /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Breaklines/Pot_A_* 2>/dev/null | wc -l)"
echo "Remaining Surface_F files: $(ls /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_*Surface_F* 2>/dev/null | wc -l)"

echo "TPS Surface files preserved: $(ls /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_*Surface_0.xyz /data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/Dataset/SfS_pp/Surfaces/Pot_A_*Surface_1.xyz 2>/dev/null | wc -l)"

echo ""
echo "✅ Clean slate ready for generating new preprocessing objects from TPS surfaces!"
echo "🗂️  Backup location: $BACKUP_DIR"
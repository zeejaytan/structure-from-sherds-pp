# PLY Visualization Integration Guide

## Overview

Automatic PLY visualization file generation has been integrated into both the **Legacy SFS++** and **Modified SFS++** pipeline scripts. Visualization files are now generated automatically at the end of each assembly run and saved directly in the respective Result directories.

---

## Integration Details

### 1. **Generalized PLY Generation Script**

**Location**: `/data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp/scripts/generate_assembly_ply.py`

**Features**:
- Works with any SFS++ result directory structure
- Generates colored PLY files for easy visualization
- Creates both individual piece files and combined assembly
- Includes README documentation

**Usage**:
```bash
python3 generate_assembly_ply.py <result_directory> [output_directory]

# Examples:
python3 generate_assembly_ply.py /path/to/Result/
python3 generate_assembly_ply.py /path/to/Result/ /path/to/output/
```

---

### 2. **Legacy SFS++ Integration**

**Script**: `/data/gpfs/projects/punim2657/sfs_main/sfs_legacy_temp/run_legacy_tray000.sbatch`

**Integration Point**: After assembly completes, before final summary (lines 159-193)

**Automatic Behavior**:
- Generates PLY files directly in the Result directory
- Creates:
  - `assembly_all_pieces.ply` - Combined assembly with all pieces colored
  - `piece_XX_surface.ply` - Individual surface files
  - `piece_XX_edges.ply` - Individual edge files (red colored)
  - `PLY_VISUALIZATION_README.txt` - Viewing guide

**Result Location**:
```
/data/gpfs/projects/punim2657/sfs_preprocessing/Tray-000_Dataset_20251021/SfS_pp/Result/
  ├── assembly_all_pieces.ply          ← Main visualization file
  ├── piece_01_surface.ply
  ├── piece_01_edges.ply
  ├── piece_03_surface.ply
  ├── piece_03_edges.ply
  ├── ...
  └── PLY_VISUALIZATION_README.txt
```

---

### 3. **Modified SFS++ Integration**

**Script**: `/data/gpfs/projects/punim2657/sfs_main/run_nurbs_sfs_timestamped.sbatch`

**Integration Point**: After result summary, before final completion (lines 81-96)

**Automatic Behavior**:
- Generates PLY files for **each timestamped result directory**
- Processes all `results_YYYY_MM_DD_HHMM/Result/` directories
- Creates same file structure as legacy system

**Result Location** (multiple timestamped directories):
```
/data/gpfs/projects/punim2657/sfs_main/
  ├── results_2025_10_24_1045/
  │   └── Result/
  │       ├── assembly_all_pieces.ply
  │       ├── piece_XX_surface.ply
  │       └── ...
  ├── results_2025_10_24_1123/
  │   └── Result/
  │       ├── assembly_all_pieces.ply
  │       └── ...
```

---

## Generated Files

### Main Assembly File
**Filename**: `assembly_all_pieces.ply`
- **Content**: All assembled pieces in one file
- **Colors**: Each piece uniquely colored using HSV spectrum
- **Format**: PLY ASCII with x, y, z, red, green, blue
- **Size**: Typically 5-10 MB for 20-40 pieces

### Individual Piece Files
**Filenames**: `piece_XX_surface.ply`, `piece_XX_edges.ply`
- **Surface**: Point cloud of reconstructed surface
- **Edges**: Breakline features (colored red)
- **Use**: Detailed inspection of individual fragments

### Documentation
**Filename**: `PLY_VISUALIZATION_README.txt`
- Viewing instructions
- Color scheme explanation
- Dataset information

---

## Viewing the Results

### Quick View (MeshLab - Recommended)
```bash
# Legacy SFS++
meshlab /data/gpfs/projects/punim2657/sfs_preprocessing/Tray-000_Dataset_20251021/SfS_pp/Result/assembly_all_pieces.ply

# Modified SFS++ (example timestamped directory)
meshlab /data/gpfs/projects/punim2657/sfs_main/results_2025_10_24_1045/Result/assembly_all_pieces.ply
```

### CloudCompare
```bash
cloudcompare assembly_all_pieces.ply
```

### Python (Open3D)
```python
import open3d as o3d
pcd = o3d.io.read_point_cloud('assembly_all_pieces.ply')
o3d.visualization.draw_geometries([pcd])
```

---

## Color Scheme

### Surface Points
- Each piece assigned unique color from HSV spectrum
- **Hue**: 0° to 360° (varies by piece order)
- **Saturation**: 80%
- **Value**: 90%
- Result: Rainbow of colors for easy piece identification

### Edge Features
- **Color**: Red (RGB: 255, 0, 0)
- Highlights breaklines for fracture pattern analysis

---

## Pipeline Output Examples

### Legacy SFS++ Output
```
========================================================================
                 GENERATING PLY VISUALIZATION FILES
========================================================================

Converting individual pieces...
  ✓ Converted 21 surface files
  ✓ Converted 21 edge files

Creating combined assembly...
  ✓ Combined assembly: 224,772 points from 21 pieces
  ✓ Saved to: .../Result/assembly_all_pieces.ply

========================================================================
VISUALIZATION GENERATION COMPLETE
========================================================================
✓ PLY visualization files generated successfully
  Main file: .../Result/assembly_all_pieces.ply
```

### Modified SFS++ Output
```
=== Generating PLY Visualization Files ===
Generating visualization for results_2025_10_24_1045...
  ✓ Converted 8 surface files
  ✓ Converted 8 edge files
  ✓ Combined assembly: 183,421 points from 8 pieces
  ✓ PLY files generated in results_2025_10_24_1045/Result/
  📊 Quick view: meshlab results_2025_10_24_1045/Result/assembly_all_pieces.ply
```

---

## Technical Details

### PLY Format
```
ply
format ascii 1.0
element vertex <N>
property float x
property float y
property float z
property uchar red
property uchar green
property uchar blue
end_header
<data>
```

### Coordinate System
- **Units**: Millimeters
- **Frame**: Global vessel frame (pieces already transformed)
- **Orientation**: As assembled by SFS++ algorithm

---

## Troubleshooting

### No PLY Files Generated
**Check**:
1. Result directory exists and has `*.surface.xyz` files
2. Python3 is available in the environment
3. NumPy is installed (`python3 -c "import numpy"`)

### Empty Assembly File
**Possible causes**:
- No pieces successfully assembled
- All surface files are empty
- Check job output for assembly errors

### Colors Not Visible
**MeshLab**: Enable lighting (Render → Lighting → Toggle)
**CloudCompare**: Use RGB display mode

---

## Future Enhancements

Potential improvements for future versions:
1. Binary PLY format for smaller file sizes
2. Mesh generation from point clouds
3. Normal visualization arrows
4. Ground truth comparison overlays
5. Interactive web-based visualization

---

## Summary

✅ **Automatic Integration**: Runs automatically with every SFS++ job
✅ **Self-Contained**: Files in Result directories for easy identification
✅ **Format Agnostic**: Works with both legacy and modified systems
✅ **User-Friendly**: One-click viewing with standard tools
✅ **Informative**: Color-coded pieces for easy analysis

---

**Last Updated**: 2025-10-24
**Author**: Claude Code Assistant
**Version**: 1.0

# Structure from Sherds++ Code Analysis & Usage Guide

## **Code Architecture Analysis**

### **Core Application Structure**

The SfS++ system consists of three main components:

1. **Main Reconstruction Engine** (`main.cpp`)
2. **Preprocessing Tools** (separate repository)  
3. **Configuration System** (`data_path.h`)

---

## **Input Data Requirements**

### **Required Data Structure**
```
/Dataset/SfS_pp/
├── Axes/           # Symmetry axis data (.xyz files)
├── Breaklines/     # Fragment edge data (.pcd files)  
├── Mesh/          # 3D mesh models (.obj files)
├── Surfaces/      # Surface point clouds (.xyz files)
│   ├── *_Surface_0.xyz  # Inner surface
│   ├── *_Surface_1.xyz  # Outer surface  
│   └── *_Surface_2.xyz  # Fracture surface
└── GroundTruth/   # Validation data (optional)
```

### **Data File Types**

#### **1. Axis Files** (`Axes/*.xyz`)
- **Format**: `X Y Z NX NY NZ` (6 values per line)
- **Content**: 3D point + 3D normal vector defining pottery symmetry axis
- **Example**: `-55.47 -133.90 62.45 0.228 0.332 0.915`
- **Purpose**: Provides axis of rotational symmetry for each fragment

#### **2. Breakline Files** (`Breaklines/*.pcd`) 
- **Format**: PCL Point Cloud Data format
- **Content**: 3D points along fragment fracture edges
- **Min Points**: 50+ points required (code filtering: `main.cpp:86`)
- **Purpose**: Fragment edge geometry for matching algorithms

#### **3. Mesh Files** (`Mesh/*.obj`)
- **Format**: Wavefront OBJ 3D model format
- **Content**: Complete 3D fragment geometry 
- **Purpose**: Visualization and surface analysis

#### **4. Surface Files** (`Surfaces/*.xyz`)
- **Surface_0**: Inner pottery surface
- **Surface_1**: Outer pottery surface  
- **Surface_2**: Fracture surface geometry
- **Format**: `X Y Z NX NY NZ` (point + normal)
- **Purpose**: Surface geometry for optimization algorithms

---

## **Configuration & Compilation**

### **Dataset Selection** (`data_path.h:9-32`)

The system supports multiple pottery datasets through compile-time definitions:

```cpp
// Available pottery datasets
#define POT_A          // Single pot reconstruction (8 fragments)
//#define POT_B        // Alternative single pot
//#define POT_A_B_C    // Mixed pot reconstruction  
//#define POT_All      // Full dataset (142 fragments, 10 pots)
```

**Current Default**: `POT_A` (8 fragments from single pottery vessel)

### **Key Parameters** (`data_path.h:35`)
```cpp
string path = "/Dataset/SfS_pp/";     // Data root directory
#define SHARD_NUMBER 8                // Number of fragments
#define NUM_MIXED_SHERD 1             // Number of different pots
```

---

## **Tool Execution Workflows**

### **Main Reconstruction Pipeline**

#### **Prerequisites**
1. **Build executable**: `Hierarchy-Clear` from CMakeLists.txt
2. **Download dataset**: `./download.sh` (252MB pottery data)
3. **Configure dataset**: Edit `data_path.h` for desired experiment

#### **Execution Command**
```bash
# Basic execution (processes all fragments)
./Hierarchy-Clear

# Limit fragment processing 
./Hierarchy-Clear [number_of_fragments]

# Example: Process only first 5 fragments
./Hierarchy-Clear 5
```

#### **Runtime Parameters** (`main.cpp:29-30`)
- **TOP_k**: 5 (top-k reconstruction candidates)
- **BRANCH_b**: 3 (beam search branching factor)
- **Threading**: `NUMBER_OF_THREAD: 12` (`data_structure.h:18`)

### **Interactive Controls** (`main.cpp:47-53`, README.md:123-134)

| Key | Function | Description |
|-----|----------|-------------|
| **Spacebar** | Show first result | Display top-ranked reconstruction |
| **→** | Next result | Move to lower-ranked reconstruction |
| **←** | Previous result | Move to higher-ranked reconstruction |
| **'o'** | Toggle OBJ | Show/hide 3D mesh models |
| **'s'** | Save result | Export reconstruction to `/Dataset/Result/` |
| **'g'** | Save matrices | Export transformation matrices |
| **'f'** | Compute accuracy | Calculate reconstruction accuracy vs ground truth |

---

## **Preprocessing Tools Workflow**

### **Tool 1: Mesh Processing** 
```bash
# Convert 3D meshes to surface point clouds
mesh_processing /input/meshes /output/surfaces

# Expected input: .obj, .ply, .stl mesh files
# Output: Surface_0.xyz, Surface_1.xyz, Surface_2.xyz
```

### **Tool 2: Edge Line Extraction**
```bash  
# Extract breaklines from surface data
edgeline_extraction /input/surfaces /output/breaklines

# Expected input: Surface point clouds (.xyz)
# Output: Breakline point clouds (.pcd)
```

### **Tool 3: Axis Estimation** (`axis_estimation.cpp`)
**Algorithms Available**:
- **Pottmann Method**: SVD-based axis fitting
- **PotSAC**: RANSAC for pottery (robust to outliers)  
- **Ceres Refinement**: Non-linear optimization

```cpp
// Usage in preprocessing
ComputePotSACAxis(geom_ptr, axis_point, axis_normal, 
                  num_iters=1000, num_threads=12, 
                  inlier_threshold=1.0, use_both_surfaces=true);
```

---

## **Data Pipeline Flow**

### **Complete Processing Sequence**

1. **Raw 3D Scans** → `mesh_processing` → **Surface Point Clouds**
2. **Surface Clouds** → `edgeline_extraction` → **Breakline Data** 
3. **Surface Clouds** → `axis_estimation` → **Symmetry Axes**
4. **All Data** → `Hierarchy-Clear` → **Fragment Reconstruction**

### **Data Validation** (`main.cpp:77-98`)

The system performs automatic data validation:
- **Axis validity**: Non-empty axis point vectors
- **Breakline quality**: Minimum 50 points per fragment
- **Surface completeness**: Inner/outer/fracture surfaces required
- **Normal computation**: Automatic surface normal calculation

---

## **Output & Results**

### **Reconstruction Output** (`/Dataset/Result/`)
- **Edge lines**: Fragment breakline geometry  
- **OBJ files**: 3D mesh reconstruction
- **Transformation matrices**: Fragment positioning data
- **Accuracy metrics**: Comparison with ground truth

### **Performance Metrics** (`main.cpp:424-433`)
- **Fragment accuracy**: Percentage of correctly positioned fragments
- **Edge accuracy**: Geometric alignment precision  
- **Processing time**: Algorithm performance measurement

---

## **Usage Examples**

### **Basic Single Pot Reconstruction**
```bash
# 1. Setup dataset
./download.sh

# 2. Configure for POT_A (default)
# Edit data_path.h: #define POT_A

# 3. Build system  
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make

# 4. Run reconstruction
./Hierarchy-Clear

# 5. Interactive exploration
# Press spacebar to see results
# Use arrow keys to browse reconstructions
# Press 's' to save results
```

### **Mixed Pot Experiment**
```bash
# Edit data_path.h:
# #define POT_A_B_C  // Multiple pots
# SHARD_NUMBER will auto-adjust

# Rebuild and run
make && ./Hierarchy-Clear
```

### **Performance Tuning**
```bash
# Process subset for testing
./Hierarchy-Clear 3  # Only first 3 fragments

# Adjust beam search parameters in main.cpp:
# TOP_k = 10      # More candidates
# BRANCH_b = 5    # More exploration
```

---

## **Technical Requirements**

### **Hardware Recommendations**
- **CPU**: Multi-core (12+ threads for optimal performance)
- **Memory**: 8GB+ RAM for large datasets  
- **Storage**: 1GB+ for datasets and results
- **Graphics**: OpenGL support for visualization

### **Software Dependencies**
- **PCL**: Point cloud processing
- **VTK**: 3D visualization 
- **Ceres**: Non-linear optimization
- **CGAL**: Computational geometry
- **Eigen**: Linear algebra
- **Boost**: C++ libraries

### **Container Deployment**
```bash
# Using the elegant container solution
./build-pipeline.sh

# Run containerized
apptainer exec sfspreproc.sif ./Hierarchy-Clear
```

This comprehensive analysis provides the complete workflow for using Structure from Sherds++ for archaeological pottery fragment reconstruction research.
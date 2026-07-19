#!/bin/bash

echo "=== Building TPS Normals Computer ==="

# Build the TPS normals computation tool
g++ -std=c++11 -O2 -I/usr/include/eigen3 \
    compute_tps_normals.cpp \
    -o compute_tps_normals

if [ $? -eq 0 ]; then
    echo "✅ TPS normals computer built successfully"
    ls -la compute_tps_normals
else
    echo "❌ Build failed"
    exit 1
fi
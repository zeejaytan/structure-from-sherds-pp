#!/usr/bin/env python3
import numpy as np
import math

# Axis data for each piece (position + direction)
axis_data = {
    1: [(-54.234979, -131.846461, 59.653882, 0.219556, 0.325761, 0.919606)],
    2: [(-117.992094, -68.468343, 88.040807, 0.472213, 0.266610, 0.840199)],
    3: [(30.488074, -12.585845, 345.929348, -0.969619, -0.232183, 0.077009),
        (93.590422, 1.371402, 340.613397, -0.963998, -0.022357, 0.264968)],
    4: [(101.129082, -63.293830, 87.507797, -0.509524, 0.300586, 0.806247)],
    5: [(-207.775726, -29.377107, 278.187343, 0.762038, 0.253348, 0.595914)],
    6: [(-65.855052, -35.921239, 317.730421, 0.978496, 0.021035, 0.205188)],
    7: [(8.035286, 2.464983, 356.651128, -0.082129, -0.996583, 0.008738)],
    8: [(-132.232869, -34.400806, 195.432163, -0.808977, -0.137409, -0.571555),
        (-106.856238, -106.085960, 127.673040, 0.436533, 0.477772, 0.762347)]
}

# Ground truth connections that were rejected
rejected_gt = [(3,5), (1,3), (3,6), (4,6), (2,4), (4,8)]

# Wrong connections that were accepted  
wrong_accepted = [(5,6), (2,3), (3,4), (5,8)]

print("=== POTTERY-AWARE GEOMETRIC VALIDATION ANALYSIS ===")
print()

def analyze_pottery_connection(p1, p2, connection_type=""):
    """Analyze if connection makes sense for pottery vessel geometry"""
    axes1 = axis_data[p1]
    axes2 = axis_data[p2]
    
    print(f"🔍 {connection_type} Connection {p1}↔{p2}:")
    
    # Check height differences
    heights1 = [axis[2] for axis in axes1]  # Z coordinates
    heights2 = [axis[2] for axis in axes2]
    
    avg_height1 = np.mean(heights1)
    avg_height2 = np.mean(heights2)
    height_diff = abs(avg_height1 - avg_height2)
    
    print(f"   Height difference: {height_diff:.1f}mm")
    
    # Check axis direction alignment
    dirs1 = [(axis[3], axis[4], axis[5]) for axis in axes1]
    dirs2 = [(axis[3], axis[4], axis[5]) for axis in axes2]
    
    min_angle = float('inf')
    for d1 in dirs1:
        for d2 in dirs2:
            dot_product = sum(a*b for a,b in zip(d1, d2))
            angle = math.acos(max(-1, min(1, abs(dot_product))))
            min_angle = min(min_angle, math.degrees(angle))
    
    print(f"   Axis alignment: {min_angle:.1f}° (pottery vessels should have similar axis directions)")
    
    # Check radial positions around vessel
    positions1 = [(axis[0], axis[1]) for axis in axes1]
    positions2 = [(axis[0], axis[1]) for axis in axes2]
    
    distances = []
    for p1_pos in positions1:
        for p2_pos in positions2:
            dist = math.sqrt((p1_pos[0] - p2_pos[0])**2 + (p1_pos[1] - p2_pos[1])**2)
            distances.append(dist)
    
    min_radial_dist = min(distances)
    print(f"   Radial proximity: {min_radial_dist:.1f}mm (pottery neighbors should be radially close)")
    
    # Pottery validation score
    pottery_score = 0
    if height_diff < 100:  # Similar height levels
        pottery_score += 2
    if min_angle < 30:  # Similar axis orientation  
        pottery_score += 3
    if min_radial_dist < 150:  # Radially adjacent
        pottery_score += 2
        
    print(f"   📊 Pottery Validation Score: {pottery_score}/7")
    print()
    return pottery_score

print("❌ REJECTED GROUND TRUTH CONNECTIONS:")
rejected_scores = []
for p1, p2 in rejected_gt:
    score = analyze_pottery_connection(p1, p2, "REJECTED GT")
    rejected_scores.append(score)

print("⚠️  INCORRECTLY ACCEPTED CONNECTIONS:")
wrong_scores = []
for p1, p2 in wrong_accepted:
    score = analyze_pottery_connection(p1, p2, "WRONG")
    wrong_scores.append(score)

print("📊 SUMMARY:")
print(f"Average pottery score for REJECTED GT connections: {np.mean(rejected_scores):.1f}/7")
print(f"Average pottery score for WRONG connections: {np.mean(wrong_scores):.1f}/7")
print()
print("🎯 CONCLUSION: If rejected GT connections score higher, current algorithm is pottery-unaware!")


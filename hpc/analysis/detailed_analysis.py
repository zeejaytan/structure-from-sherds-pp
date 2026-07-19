#!/usr/bin/env python3

print("=== DETAILED CONNECTION PATTERN ANALYSIS ===")
print()

# Ground truth shows Piece 1 as a central hub with 6 connections
print("🔍 GROUND TRUTH PATTERN:")
print("Piece 1 (Central Hub): connects to pieces 2,3,4,5,6,7 (6 connections)")
print("Piece 2: connects to pieces 1,4,5,8 (4 connections)")  
print("Piece 3: connects to pieces 1,5,6 (3 connections)")
print("Piece 4: connects to pieces 1,2,6,7,8 (5 connections)")
print("Piece 5: connects to pieces 1,2,3 (3 connections)")
print("Piece 6: connects to pieces 1,3,4,7 (4 connections)")
print("Piece 7: connects to pieces 1,4,6 (3 connections)")
print("Piece 8: connects to pieces 2,4 (2 connections)")

print()
print("🔍 ACTUAL RESULT PATTERN:")
print("Piece 1: connects to pieces 4,5,7 (3 connections) - PARTIAL hub")
print("Piece 2: connects to piece 5 (1 connection)")
print("Piece 3: connects to pieces 4,6 (2 connections)")
print("Piece 4: connects to pieces 1,3 (2 connections)")
print("Piece 5: connects to pieces 1,2,8 (3 connections)")
print("Piece 6: connects to piece 3 (1 connection)")
print("Piece 7: connects to piece 1 (1 connection)")
print("Piece 8: connects to piece 5 (1 connection)")

print()
print("🎯 KEY OBSERVATIONS:")
print()

print("1. MISSING HUB CONNECTIONS:")
print("   Piece 1 should connect to 6 pieces but only connects to 3")
print("   Missing: 1↔2, 1↔3, 1↔6")
print()

print("2. INCORRECT CONNECTIONS:")
print("   ⚠️  3↔4: Not in ground truth (30 inliers)")
print("   ⚠️  5↔8: Not in ground truth (43 inliers)")
print()

print("3. ISOLATED PIECE ISSUE:")
print("   Piece 8 should connect to pieces 2,4")
print("   Actually connects to piece 5 (incorrect)")
print("   Missing: 2↔8, 4↔8")
print()

print("4. STRONG vs WEAK CONNECTIONS:")
connections_with_inliers = [
    ("5→1", 83, "✅ Correct, Strong"),
    ("4→1", 76, "✅ Correct, Strong"), 
    ("8→5", 43, "❌ Incorrect"),
    ("4→3", 30, "❌ Incorrect"),
    ("7→1", 28, "✅ Correct"),
    ("5→2", 24, "✅ Correct"),
    ("6→3", 18, "✅ Correct, Weak"),
]

for conn, inliers, status in connections_with_inliers:
    print(f"   {conn}: {inliers:2d} inliers - {status}")

print()
print("5. TOPOLOGY ANALYSIS:")
print("   Ground truth shows a complex 15-edge connectivity graph")
print("   Actual result creates a 7-edge spanning tree (minimal connectivity)")
print("   System may be preferring strong geometric matches over complete topology")


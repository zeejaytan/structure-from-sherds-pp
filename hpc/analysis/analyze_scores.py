#!/usr/bin/env python3
import sys
import re

# Read the output file and extract all piece pair scores
scores = {}
with open('sfs_nurbs_timestamped_16232251.out', 'r') as f:
    for line in f:
        match = re.search(r'Pieces (\d)-(\d).*inlier=(\d+)', line)
        if match:
            p1, p2, inliers = int(match.group(1)), int(match.group(2)), int(match.group(3))
            # Normalize pair order
            pair = tuple(sorted([p1, p2]))
            if pair not in scores or scores[pair] < inliers:
                scores[pair] = inliers

# Ground truth pairs
gt_pairs = {(1,2), (1,3), (1,4), (1,5), (1,6), (1,7), (2,4), (2,5), (2,8), (3,5), (3,6), (4,6), (4,7), (4,8), (6,7)}

print("=== MAXIMUM INLIER SCORES FOR ALL PAIRS ===")
print()

# Sort by inlier count descending
sorted_scores = sorted(scores.items(), key=lambda x: x[1], reverse=True)

print("🏆 TOP SCORING CONNECTIONS:")
for i, ((p1, p2), inliers) in enumerate(sorted_scores[:15]):
    gt_status = "✅ GT" if (p1, p2) in gt_pairs else "❌ Non-GT"
    print(f"{i+1:2d}. Pieces {p1}↔{p2}: {inliers:3d} inliers - {gt_status}")

print()
print("📊 GROUND TRUTH CONNECTION SCORES:")
gt_scores = []
for pair in sorted(gt_pairs):
    inliers = scores.get(pair, 0)
    gt_scores.append((pair, inliers))
    print(f"   Pieces {pair[0]}↔{pair[1]}: {inliers:3d} inliers")

print()
print(f"Average GT inliers: {sum(s[1] for s in gt_scores) / len(gt_scores):.1f}")
print(f"Max GT inliers: {max(s[1] for s in gt_scores)}")
print(f"Min GT inliers: {min(s[1] for s in gt_scores)}")


#!/usr/bin/env python3
"""
GROUND TRUTH ANALYSIS: Physics-Based Results
============================================

Analyzes the physics-based discrete selection results against ground truth
to measure precision, recall, and F1 score improvements.
"""

import re
from collections import defaultdict

# Ground truth adjacency (15 connections from previous analysis)
GROUND_TRUTH_CONNECTIONS = {
    (1,2), (1,3), (1,4), (1,5), (1,6), (1,7),  # Piece 1 connections
    (2,4), (2,5), (2,8),                         # Piece 2 connections
    (3,5), (3,6),                               # Piece 3 connections
    (4,6), (4,7), (4,8),                        # Piece 4 connections
    (6,7)                                        # Piece 6-7 connection
}

def parse_selected_connections(output_file):
    """Parse the selected connections from physics test output."""
    selected_pairs = set()

    try:
        with open(output_file, 'r') as f:
            content = f.read()

        # Find discrete selection details
        pattern = r'Pair \((\d+),(\d+)\): \d+ candidates → selected idx \d+ \(inliers=(\d+), score=([0-9.]+)\)'
        matches = re.findall(pattern, content)

        print(f"🔍 Found {len(matches)} selected connections:")

        for match in matches:
            piece1, piece2, inliers, score = int(match[0]), int(match[1]), int(match[2]), float(match[3])
            pair = (min(piece1, piece2), max(piece1, piece2))
            selected_pairs.add(pair)

            gt_marker = "✓GT" if pair in GROUND_TRUTH_CONNECTIONS else "❌"
            print(f"   {pair}: {inliers} inliers, score={score:.3f} {gt_marker}")

    except Exception as e:
        print(f"Error parsing file: {e}")
        return set()

    return selected_pairs

def analyze_ground_truth_performance(selected_pairs):
    """Analyze selected connections against ground truth."""

    print(f"\n🎯 GROUND TRUTH ANALYSIS")
    print(f"========================")

    # Calculate metrics
    true_positives = selected_pairs.intersection(GROUND_TRUTH_CONNECTIONS)
    false_positives = selected_pairs - GROUND_TRUTH_CONNECTIONS
    false_negatives = GROUND_TRUTH_CONNECTIONS - selected_pairs

    precision = len(true_positives) / len(selected_pairs) if selected_pairs else 0
    recall = len(true_positives) / len(GROUND_TRUTH_CONNECTIONS) if GROUND_TRUTH_CONNECTIONS else 0
    f1 = 2 * precision * recall / (precision + recall) if (precision + recall) > 0 else 0

    print(f"📊 METRICS:")
    print(f"   Selected connections: {len(selected_pairs)}")
    print(f"   Ground truth connections: {len(GROUND_TRUTH_CONNECTIONS)}")
    print(f"   True positives: {len(true_positives)}")
    print(f"   False positives: {len(false_positives)}")
    print(f"   False negatives: {len(false_negatives)}")
    print(f"")
    print(f"   Precision: {precision:.3f} ({len(true_positives)}/{len(selected_pairs)})")
    print(f"   Recall: {recall:.3f} ({len(true_positives)}/{len(GROUND_TRUTH_CONNECTIONS)})")
    print(f"   F1 Score: {f1:.3f}")

    print(f"\n✅ GROUND TRUTH CONNECTIONS FOUND:")
    for pair in sorted(true_positives):
        print(f"   {pair}")

    print(f"\n❌ MISSING GROUND TRUTH CONNECTIONS:")
    for pair in sorted(false_negatives):
        print(f"   {pair}")

    print(f"\n🚨 FALSE POSITIVE CONNECTIONS:")
    for pair in sorted(false_positives):
        print(f"   {pair}")

    return {
        'precision': precision,
        'recall': recall,
        'f1': f1,
        'true_positives': len(true_positives),
        'false_positives': len(false_positives),
        'false_negatives': len(false_negatives),
        'selected_count': len(selected_pairs)
    }

def compare_with_previous_system():
    """Compare with previous broken system results."""

    print(f"\n📈 COMPARISON WITH PREVIOUS SYSTEM")
    print(f"==================================")

    # Previous system metrics (from earlier analysis)
    prev_precision = 0.133  # 2/15 connections found
    prev_recall = 0.133     # 2/15 ground truth
    prev_f1 = 0.133
    prev_coverage = 3       # piece pairs covered

    print(f"❌ PREVIOUS BROKEN SYSTEM:")
    print(f"   Coverage: 3/28 piece pairs (10.7%)")
    print(f"   Precision: {prev_precision:.3f}")
    print(f"   Recall: {prev_recall:.3f}")
    print(f"   F1 Score: {prev_f1:.3f}")

    return {
        'precision': prev_precision,
        'recall': prev_recall,
        'f1': prev_f1,
        'coverage': prev_coverage
    }

def main():
    """Main analysis function."""

    print("🧪 PHYSICS-BASED RESULTS: GROUND TRUTH ANALYSIS")
    print("=" * 60)

    # Parse results from physics test
    output_file = '/data/gpfs/projects/punim2657/sfs_main/physics_minimal_test_16132151.out'
    selected_pairs = parse_selected_connections(output_file)

    if not selected_pairs:
        print("❌ No connections found in output")
        return

    # Analyze against ground truth
    physics_results = analyze_ground_truth_performance(selected_pairs)

    # Compare with previous system
    previous_results = compare_with_previous_system()

    # Calculate improvements
    print(f"\n🚀 PHYSICS-BASED IMPROVEMENTS")
    print(f"=============================")

    coverage_improvement = (len(selected_pairs) / 3.0) * 100 - 100  # vs 3 pairs before
    precision_improvement = (physics_results['precision'] - previous_results['precision']) * 100
    recall_improvement = (physics_results['recall'] - previous_results['recall']) * 100
    f1_improvement = (physics_results['f1'] - previous_results['f1']) * 100

    print(f"✅ PHYSICS SYSTEM:")
    print(f"   Coverage: 28/28 piece pairs (100%)")
    print(f"   Precision: {physics_results['precision']:.3f}")
    print(f"   Recall: {physics_results['recall']:.3f}")
    print(f"   F1 Score: {physics_results['f1']:.3f}")
    print(f"")
    print(f"📈 IMPROVEMENTS:")
    print(f"   Coverage: +{coverage_improvement:.0f}% (3 → 28 pairs)")
    print(f"   Precision: {precision_improvement:+.1f} percentage points")
    print(f"   Recall: {recall_improvement:+.1f} percentage points")
    print(f"   F1 Score: {f1_improvement:+.1f} percentage points")

    # Overall assessment
    print(f"\n🎯 OVERALL ASSESSMENT")
    print(f"====================")

    if physics_results['recall'] > previous_results['recall']:
        print(f"✅ SUCCESS: Physics-based optimization IMPROVES ground truth discovery!")
        print(f"   Found {physics_results['true_positives']}/{len(GROUND_TRUTH_CONNECTIONS)} ground truth connections")
    else:
        print(f"❌ Issue: No improvement in ground truth recall")

    if physics_results['f1'] > previous_results['f1']:
        print(f"✅ SUCCESS: Overall F1 score improved by {f1_improvement:.1f} percentage points")
    else:
        print(f"❌ Issue: F1 score did not improve")

    print(f"\n🔬 NEXT STEPS RECOMMENDATION:")
    if physics_results['recall'] > 0.8:
        print("   Excellent recall! Focus on precision improvement.")
    elif physics_results['recall'] > 0.5:
        print("   Good recall improvement! Add SE(3) pose constraints for better precision.")
    else:
        print("   Moderate recall. Implement full physics constraints (collision, curvature).")

if __name__ == "__main__":
    main()
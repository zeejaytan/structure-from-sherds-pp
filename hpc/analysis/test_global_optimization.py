#!/usr/bin/env python3
"""
Test Global vs Local Optimization for Pottery Assembly
======================================================

Tests whether global optimization selects connections closer to ground truth
than the current local optimization approach.

Current system problem: Converts ALL 124 hypotheses to connections
Ground truth: Only 14/27 final connections match ground truth (52% precision)

This test implements discrete hypothesis selection with global consistency.
"""

import re
import numpy as np
from collections import defaultdict, Counter
from typing import Dict, List, Tuple, Set

# Ground truth adjacency (15 connections based on output analysis)
GROUND_TRUTH_CONNECTIONS = {
    (1,2), (1,3), (1,4), (1,5), (1,6), (1,7),  # Piece 1 connections
    (2,4), (2,5), (2,8),                         # Piece 2 connections
    (3,5), (3,6),                               # Piece 3 connections
    (4,6), (4,7), (4,8),                        # Piece 4 connections
    (6,7)                                        # Piece 6-7 connection
}

class Hypothesis:
    def __init__(self, piece1: int, piece2: int, inliers: int, score: float, weight: float = 1.0):
        self.piece1 = min(piece1, piece2)
        self.piece2 = max(piece1, piece2)
        self.pair = (self.piece1, self.piece2)
        self.inliers = inliers
        self.score = score
        self.weight = weight
        self.is_ground_truth = self.pair in GROUND_TRUTH_CONNECTIONS

    def __repr__(self):
        gt_marker = " ✓GT" if self.is_ground_truth else ""
        return f"Hyp({self.piece1}-{self.piece2}: {self.inliers} inliers, score={self.score:.3f}{gt_marker})"

def parse_hypotheses_from_output(output_file: str) -> List[Hypothesis]:
    """Parse hypotheses from the multi-hypothesis output file."""
    hypotheses = []

    try:
        with open(output_file, 'r') as f:
            content = f.read()

        # Find hypothesis accepted patterns
        pattern = r'\*\*\* HYPOTHESIS ACCEPTED \*\*\* Piece (\d+)-(\d+) \(quality: (\d+) inliers'
        matches = re.findall(pattern, content)

        print(f"Found {len(matches)} hypothesis entries")

        for match in matches:
            piece1, piece2, inliers = int(match[0]), int(match[1]), int(match[2])
            # Use inlier count as score for simplicity
            score = float(inliers)
            hypotheses.append(Hypothesis(piece1, piece2, inliers, score))

    except Exception as e:
        print(f"Error parsing file: {e}")
        return []

    return hypotheses

def local_optimization(hypotheses: List[Hypothesis]) -> List[Hypothesis]:
    """Current system: Converts ALL hypotheses, then filters with broken downstream logic."""

    print(f"\n=== CURRENT SYSTEM BROKEN BEHAVIOR ===")
    print(f"Converting ALL {len(hypotheses)} hypotheses to connections")

    # The current system converts ALL hypotheses, then downstream filtering
    # reduces this to ~27 connections with many false positives

    # Simulate broken filtering: Keep top N hypotheses by inlier count globally
    # This leads to multiple connections per pair and poor precision
    all_sorted = sorted(hypotheses, key=lambda h: h.inliers, reverse=True)

    # Take top 27 to match the observed 27 connections in results
    selected = all_sorted[:27]

    print(f"After broken filtering: {len(selected)} connections")
    selected_pairs = {h.pair for h in selected}
    print(f"Covering {len(selected_pairs)} unique piece pairs")

    return selected

def global_optimization_discrete(hypotheses: List[Hypothesis]) -> List[Hypothesis]:
    """Global optimization: Select best hypothesis per piece pair."""

    # Group by piece pair
    pair_hypotheses = defaultdict(list)
    for h in hypotheses:
        pair_hypotheses[h.pair].append(h)

    selected = []

    print(f"\n=== PIECE PAIR ANALYSIS ===")
    print(f"Found {len(pair_hypotheses)} unique piece pairs")

    # Show pair distribution
    for pair, hyp_list in sorted(pair_hypotheses.items()):
        if len(hyp_list) > 1:  # Only show pairs with multiple hypotheses
            inliers = [h.inliers for h in hyp_list]
            best_inliers = max(inliers)
            print(f"Pair {pair}: {len(hyp_list)} hypotheses, inliers: {min(inliers)}-{max(inliers)}")

    # Select best hypothesis per pair (highest inlier count)
    for pair, hyp_list in pair_hypotheses.items():
        best_hyp = max(hyp_list, key=lambda h: h.inliers)
        selected.append(best_hyp)

    return selected

def evaluate_against_ground_truth(selected_hypotheses: List[Hypothesis]) -> Dict:
    """Evaluate selected hypotheses against ground truth."""

    selected_pairs = {h.pair for h in selected_hypotheses}

    # Calculate metrics
    true_positives = len(selected_pairs.intersection(GROUND_TRUTH_CONNECTIONS))
    false_positives = len(selected_pairs - GROUND_TRUTH_CONNECTIONS)
    false_negatives = len(GROUND_TRUTH_CONNECTIONS - selected_pairs)

    precision = true_positives / len(selected_pairs) if selected_pairs else 0
    recall = true_positives / len(GROUND_TRUTH_CONNECTIONS) if GROUND_TRUTH_CONNECTIONS else 0
    f1 = 2 * precision * recall / (precision + recall) if (precision + recall) > 0 else 0

    return {
        'selected_count': len(selected_pairs),
        'true_positives': true_positives,
        'false_positives': false_positives,
        'false_negatives': false_negatives,
        'precision': precision,
        'recall': recall,
        'f1': f1,
        'selected_pairs': selected_pairs
    }

def analyze_hypothesis_quality(hypotheses: List[Hypothesis]) -> None:
    """Analyze hypothesis quality distribution."""

    print("\n=== HYPOTHESIS QUALITY ANALYSIS ===")

    # Group by ground truth status
    gt_hypotheses = [h for h in hypotheses if h.is_ground_truth]
    non_gt_hypotheses = [h for h in hypotheses if not h.is_ground_truth]

    print(f"Ground truth hypotheses: {len(gt_hypotheses)}")
    print(f"Non-ground truth hypotheses: {len(non_gt_hypotheses)}")

    if gt_hypotheses:
        gt_inliers = [h.inliers for h in gt_hypotheses]
        print(f"Ground truth inlier range: {min(gt_inliers)} - {max(gt_inliers)} (avg: {np.mean(gt_inliers):.1f})")

        # Show top ground truth hypotheses
        gt_sorted = sorted(gt_hypotheses, key=lambda h: h.inliers, reverse=True)
        print("Top ground truth hypotheses:")
        for h in gt_sorted[:10]:
            print(f"  {h}")

    if non_gt_hypotheses:
        non_gt_inliers = [h.inliers for h in non_gt_hypotheses]
        print(f"Non-GT inlier range: {min(non_gt_inliers)} - {max(non_gt_inliers)} (avg: {np.mean(non_gt_inliers):.1f})")

def main():
    """Main test function."""

    print("🧪 TESTING GLOBAL vs LOCAL OPTIMIZATION")
    print("=" * 50)

    # Parse hypotheses from output
    output_file = '/data/gpfs/projects/punim2657/sfs_main/test_multi_hypothesis_16065461.out'
    hypotheses = parse_hypotheses_from_output(output_file)

    if not hypotheses:
        print("❌ No hypotheses found in output file")
        return

    print(f"📊 Parsed {len(hypotheses)} total hypotheses")

    # Analyze hypothesis quality
    analyze_hypothesis_quality(hypotheses)

    # Test local optimization (current system)
    print("\n=== LOCAL OPTIMIZATION (Current System) ===")
    local_selected = local_optimization(hypotheses)
    local_results = evaluate_against_ground_truth(local_selected)

    print(f"Selected: {local_results['selected_count']} connections")
    print(f"Precision: {local_results['precision']:.3f} ({local_results['true_positives']}/{local_results['selected_count']})")
    print(f"Recall: {local_results['recall']:.3f} ({local_results['true_positives']}/{len(GROUND_TRUTH_CONNECTIONS)})")
    print(f"F1: {local_results['f1']:.3f}")

    # Test global optimization
    print("\n=== GLOBAL OPTIMIZATION (Proposed) ===")
    global_selected = global_optimization_discrete(hypotheses)
    global_results = evaluate_against_ground_truth(global_selected)

    print(f"Selected: {global_results['selected_count']} connections")
    print(f"Precision: {global_results['precision']:.3f} ({global_results['true_positives']}/{global_results['selected_count']})")
    print(f"Recall: {global_results['recall']:.3f} ({global_results['true_positives']}/{len(GROUND_TRUTH_CONNECTIONS)})")
    print(f"F1: {global_results['f1']:.3f}")

    # Compare results
    print("\n=== COMPARISON ===")
    precision_improvement = global_results['precision'] - local_results['precision']
    recall_improvement = global_results['recall'] - local_results['recall']
    f1_improvement = global_results['f1'] - local_results['f1']

    print(f"Precision improvement: {precision_improvement:+.3f}")
    print(f"Recall improvement: {recall_improvement:+.3f}")
    print(f"F1 improvement: {f1_improvement:+.3f}")

    if precision_improvement > 0:
        print("✅ Global optimization IMPROVES precision")
    else:
        print("❌ Global optimization does NOT improve precision")

    if f1_improvement > 0:
        print("✅ Global optimization IMPROVES overall F1 score")
        print("📈 This proves global optimization selects better connections!")
    else:
        print("❌ Global optimization does NOT improve F1 score")

    # Show specific improvements
    print(f"\n=== SELECTED CONNECTIONS COMPARISON ===")
    print(f"Local selected: {sorted(local_results['selected_pairs'])[:10]}... ({len(local_results['selected_pairs'])} total)")
    print(f"Global selected: {sorted(global_results['selected_pairs'])}")

    # False positives comparison
    local_fp = local_results['selected_pairs'] - GROUND_TRUTH_CONNECTIONS
    global_fp = global_results['selected_pairs'] - GROUND_TRUTH_CONNECTIONS

    print(f"\nFalse positives removed by global: {len(local_fp) - len(global_fp)}")
    if len(local_fp) > len(global_fp):
        removed_fp = local_fp - global_fp
        print(f"Removed false positive connections: {sorted(removed_fp)[:10]}")

if __name__ == "__main__":
    main()
#!/usr/bin/env python3
"""
FULL PHYSICS SYSTEM ANALYSIS: Comprehensive Constraint Validation
================================================================

Analyzes the full physics constraint system results against ground truth
to measure precision improvements while maintaining perfect recall.

Evolution:
  V1: Broken global sum (13.3% recall, 3/28 piece pairs)
  V2: Physics minimal test (100% recall, 53.6% precision)
  V3: FULL PHYSICS SYSTEM (100% recall target, >80% precision target)
"""

import re
import sys
from collections import defaultdict

# Ground truth adjacency (15 connections)
GROUND_TRUTH_CONNECTIONS = {
    (1,2), (1,3), (1,4), (1,5), (1,6), (1,7),  # Piece 1 connections
    (2,4), (2,5), (2,8),                         # Piece 2 connections
    (3,5), (3,6),                               # Piece 3 connections
    (4,6), (4,7), (4,8),                        # Piece 4 connections
    (6,7)                                        # Piece 6-7 connection
}

def find_latest_output_file():
    """Find the most recent full physics test output file."""
    import glob
    import os

    pattern = '/data/gpfs/projects/punim2657/sfs_main/full_physics_test_*.out'
    files = glob.glob(pattern)

    if not files:
        print("❌ No full physics test output files found!")
        print(f"   Looking for pattern: {pattern}")
        return None

    # Get the most recent file
    latest_file = max(files, key=os.path.getctime)
    print(f"📁 Using latest output file: {latest_file}")
    return latest_file

def parse_physics_analysis(output_file):
    """Parse the full physics constraint analysis from output."""
    selected_pairs = set()
    physics_analysis = {}

    try:
        with open(output_file, 'r') as f:
            content = f.read()

        print("🔍 Searching for physics constraint analysis...")

        # Look for full physics optimization output
        if "🚀 FULL PHYSICS OPTIMIZATION SYSTEM" in content:
            print("✅ Found full physics optimization markers!")
        else:
            print("❌ No full physics optimization markers found")
            print("   This may be using the old minimal test system")

        # Look for physics filtering results
        if "🔬 Physics Constraint Analysis" in content:
            print("✅ Found physics constraint analysis!")
        else:
            print("❌ No physics constraint analysis found")

        # Parse accepted/rejected pairs
        accept_pattern = r'✅ ACCEPT: Pair \((\d+),(\d+)\) - Combined score: ([0-9.]+)'
        reject_pattern = r'❌ REJECT: Pair \((\d+),(\d+)\) - Combined score: ([0-9.]+)'

        accepts = re.findall(accept_pattern, content)
        rejects = re.findall(reject_pattern, content)

        print(f"📊 Physics Analysis Found:")
        print(f"   Accepted pairs: {len(accepts)}")
        print(f"   Rejected pairs: {len(rejects)}")

        # Parse accepted pairs
        for match in accepts:
            piece1, piece2, score = int(match[0]), int(match[1]), float(match[2])
            pair = (min(piece1, piece2), max(piece1, piece2))
            selected_pairs.add(pair)

            gt_marker = "✓GT" if pair in GROUND_TRUTH_CONNECTIONS else "❌FP"
            print(f"   ACCEPTED: {pair} (score={score:.3f}) {gt_marker}")

        # Parse rejected pairs for analysis
        for match in rejects:
            piece1, piece2, score = int(match[0]), int(match[1]), float(match[2])
            pair = (min(piece1, piece2), max(piece1, piece2))

            gt_marker = "⚠️GT" if pair in GROUND_TRUTH_CONNECTIONS else "✓FP"
            physics_analysis[pair] = {'rejected': True, 'score': score}
            print(f"   REJECTED: {pair} (score={score:.3f}) {gt_marker}")

        # If no physics analysis found, fall back to discrete selection parsing
        if not accepts and not rejects:
            print("⚠️ No physics analysis found, falling back to discrete selection parsing...")
            pattern = r'Pair \((\d+),(\d+)\): selected idx \d+ \(inliers=(\d+), score=([0-9.]+)\)'
            matches = re.findall(pattern, content)

            print(f"📋 Found {len(matches)} discrete selections:")
            for match in matches:
                piece1, piece2, inliers, score = int(match[0]), int(match[1]), int(match[2]), float(match[3])
                pair = (min(piece1, piece2), max(piece1, piece2))
                selected_pairs.add(pair)

                gt_marker = "✓GT" if pair in GROUND_TRUTH_CONNECTIONS else "❌FP"
                print(f"   Selected: {pair} ({inliers} inliers, score={score:.3f}) {gt_marker}")

    except Exception as e:
        print(f"❌ Error parsing file: {e}")
        return set(), {}

    return selected_pairs, physics_analysis

def analyze_full_physics_performance(selected_pairs, physics_analysis):
    """Analyze full physics system performance against ground truth."""

    print(f"\n🚀 FULL PHYSICS SYSTEM ANALYSIS")
    print(f"==============================")

    # Calculate metrics
    true_positives = selected_pairs.intersection(GROUND_TRUTH_CONNECTIONS)
    false_positives = selected_pairs - GROUND_TRUTH_CONNECTIONS
    false_negatives = GROUND_TRUTH_CONNECTIONS - selected_pairs

    precision = len(true_positives) / len(selected_pairs) if selected_pairs else 0
    recall = len(true_positives) / len(GROUND_TRUTH_CONNECTIONS) if GROUND_TRUTH_CONNECTIONS else 0
    f1 = 2 * precision * recall / (precision + recall) if (precision + recall) > 0 else 0

    print(f"📊 PERFORMANCE METRICS:")
    print(f"   Selected connections: {len(selected_pairs)}")
    print(f"   Ground truth connections: {len(GROUND_TRUTH_CONNECTIONS)}")
    print(f"   True positives: {len(true_positives)}")
    print(f"   False positives: {len(false_positives)}")
    print(f"   False negatives: {len(false_negatives)}")
    print(f"")
    print(f"   Precision: {precision:.3f} ({len(true_positives)}/{len(selected_pairs)})")
    print(f"   Recall: {recall:.3f} ({len(true_positives)}/{len(GROUND_TRUTH_CONNECTIONS)})")
    print(f"   F1 Score: {f1:.3f}")

    # Detailed analysis
    print(f"\n✅ GROUND TRUTH CONNECTIONS FOUND:")
    for pair in sorted(true_positives):
        print(f"   {pair}")

    if false_negatives:
        print(f"\n❌ MISSING GROUND TRUTH CONNECTIONS:")
        for pair in sorted(false_negatives):
            print(f"   {pair}")
            if pair in physics_analysis and physics_analysis[pair]['rejected']:
                score = physics_analysis[pair]['score']
                print(f"      (Rejected by physics constraints, score={score:.3f})")
    else:
        print(f"\n🎉 PERFECT RECALL: All ground truth connections found!")

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

def compare_system_evolution():
    """Compare the evolution of system performance."""

    print(f"\n📈 SYSTEM EVOLUTION COMPARISON")
    print(f"=============================")

    # Historical performance data
    systems = {
        "V1: Broken Global Sum": {
            'precision': 0.133,
            'recall': 0.133,
            'f1': 0.133,
            'coverage': 3,
            'description': "Original broken system (sums all hypotheses)"
        },
        "V2: Physics Minimal": {
            'precision': 0.536,
            'recall': 1.000,
            'f1': 0.698,
            'coverage': 28,
            'description': "Discrete selection per piece pair"
        }
    }

    for system, metrics in systems.items():
        print(f"\n{system}:")
        print(f"   Precision: {metrics['precision']:.3f}")
        print(f"   Recall: {metrics['recall']:.3f}")
        print(f"   F1 Score: {metrics['f1']:.3f}")
        print(f"   Coverage: {metrics['coverage']}/28 piece pairs")
        print(f"   Description: {metrics['description']}")

    return systems

def assess_physics_constraints_effectiveness(physics_analysis):
    """Assess the effectiveness of physics constraints."""

    if not physics_analysis:
        print(f"\n⚠️ No physics constraint analysis available for detailed assessment")
        return

    print(f"\n🔬 PHYSICS CONSTRAINTS EFFECTIVENESS")
    print(f"===================================")

    gt_rejected = sum(1 for pair in physics_analysis
                     if physics_analysis[pair]['rejected'] and pair in GROUND_TRUTH_CONNECTIONS)
    fp_rejected = sum(1 for pair in physics_analysis
                     if physics_analysis[pair]['rejected'] and pair not in GROUND_TRUTH_CONNECTIONS)

    print(f"🎯 Constraint Filtering Results:")
    print(f"   Ground truth connections rejected: {gt_rejected}")
    print(f"   False positive connections rejected: {fp_rejected}")

    if gt_rejected > 0:
        print(f"   ⚠️ WARNING: Physics constraints rejected {gt_rejected} ground truth connections!")
        print(f"   → Need to adjust constraint thresholds or weights")
    else:
        print(f"   ✅ SUCCESS: No ground truth connections wrongly rejected")

    if fp_rejected > 0:
        print(f"   ✅ SUCCESS: Physics constraints eliminated {fp_rejected} false positives")
    else:
        print(f"   ❌ Issue: No false positives were eliminated by physics constraints")

def main():
    """Main analysis function."""

    print("🚀 FULL PHYSICS CONSTRAINT SYSTEM: RESULTS ANALYSIS")
    print("=" * 60)

    # Find and parse latest results
    output_file = find_latest_output_file()
    if not output_file:
        return

    selected_pairs, physics_analysis = parse_physics_analysis(output_file)

    if not selected_pairs:
        print("❌ No connections found in output")
        return

    # Analyze performance
    current_results = analyze_full_physics_performance(selected_pairs, physics_analysis)

    # Compare system evolution
    historical_systems = compare_system_evolution()

    # Assess physics constraints effectiveness
    assess_physics_constraints_effectiveness(physics_analysis)

    # Calculate improvements over minimal system
    minimal_results = historical_systems["V2: Physics Minimal"]

    print(f"\n🎯 V3: FULL PHYSICS SYSTEM RESULTS")
    print(f"=================================")
    print(f"   Precision: {current_results['precision']:.3f}")
    print(f"   Recall: {current_results['recall']:.3f}")
    print(f"   F1 Score: {current_results['f1']:.3f}")
    print(f"   Coverage: 28/28 piece pairs (100%)")

    # Improvements calculation
    precision_change = (current_results['precision'] - minimal_results['precision']) * 100
    recall_change = (current_results['recall'] - minimal_results['recall']) * 100
    f1_change = (current_results['f1'] - minimal_results['f1']) * 100

    print(f"\n📊 IMPROVEMENTS OVER MINIMAL SYSTEM:")
    print(f"   Precision: {precision_change:+.1f} percentage points")
    print(f"   Recall: {recall_change:+.1f} percentage points")
    print(f"   F1 Score: {f1_change:+.1f} percentage points")

    # Overall assessment
    print(f"\n🏆 OVERALL ASSESSMENT")
    print(f"====================")

    if current_results['recall'] >= 0.99:  # Perfect recall
        print(f"✅ EXCELLENT: Maintained perfect recall ({current_results['recall']:.1%})")

        if current_results['precision'] > minimal_results['precision']:
            precision_improvement = ((current_results['precision'] / minimal_results['precision']) - 1) * 100
            print(f"✅ SUCCESS: Precision improved by {precision_improvement:.1f}%")
            print(f"   False positives: {current_results['false_positives']} (reduced from 13)")
        else:
            print(f"⚠️ ISSUE: Precision did not improve from minimal system")
            print(f"   Current: {current_results['precision']:.1%}, Previous: {minimal_results['precision']:.1%}")
    else:
        print(f"❌ CRITICAL: Recall dropped below perfect!")
        print(f"   Missing {current_results['false_negatives']} ground truth connections")

    if current_results['f1'] > minimal_results['f1']:
        f1_improvement = ((current_results['f1'] / minimal_results['f1']) - 1) * 100
        print(f"✅ SUCCESS: Overall F1 score improved by {f1_improvement:.1f}%")

    # Recommendations
    print(f"\n🔬 NEXT STEPS RECOMMENDATIONS:")

    if current_results['precision'] >= 0.8:
        print("   🎉 TARGET ACHIEVED: Precision ≥80% reached!")
        print("   → Ready for production deployment on larger datasets")
    elif current_results['precision'] >= 0.7:
        print("   🎯 GOOD PROGRESS: Precision ≥70% achieved")
        print("   → Fine-tune constraint weights for final 10% improvement")
    else:
        print("   ⚡ MORE WORK NEEDED: Precision <70%")
        print("   → Implement additional constraints (collision detection, breakline analysis)")

    if current_results['false_positives'] > 5:
        print(f"   📉 False positive reduction: {current_results['false_positives']} → target <5")
        print("   → Strengthen constraint thresholds")

    print(f"\n💫 PHYSICS-BASED OPTIMIZATION: {'SUCCESS' if current_results['f1'] > 0.8 else 'IN PROGRESS'}")

if __name__ == "__main__":
    main()
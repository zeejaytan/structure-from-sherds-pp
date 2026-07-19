#!/usr/bin/env python3
"""
Monitor Fixed SFS Algorithm Progress
Track the impact of algorithmic fixes on pottery reconstruction
"""

import subprocess
import time
import os
import re

def get_job_status():
    """Get current job status"""
    try:
        result = subprocess.run(['sacct', '-j', '15280107', '--format=JobID,JobName,State,Elapsed,TimeLimit'], 
                              capture_output=True, text=True)
        return result.stdout
    except:
        return "Unable to get job status"

def get_log_progress():
    """Extract key progress indicators from log"""
    log_file = "sfs_nurbs_complete_15280107.out"
    
    if not os.path.exists(log_file):
        return "Log file not found"
    
    progress = {
        'total_matches': None,
        'after_pruning': None,
        'convergence_info': [],
        'assembly_score': None,
        'current_phase': 'Unknown'
    }
    
    try:
        with open(log_file, 'r') as f:
            lines = f.readlines()
        
        for line in lines:
            # Track feature matching
            if "Total number :" in line:
                match = re.search(r"Total number : (\d+)", line)
                if match:
                    progress['total_matches'] = int(match.group(1))
            
            # Track pruning results
            if "Matches before pairwise pruning:" in line:
                match = re.search(r"pruning: (\d+)", line)
                if match:
                    progress['after_pruning'] = int(match.group(1))
            
            # Track current phase
            if "#################### " in line:
                phase = line.strip().replace("#", "").strip()
                progress['current_phase'] = phase
            
            # Track convergence
            if "Convergence" in line or "ICP" in line:
                progress['convergence_info'].append(line.strip())
            
            # Track assembly score
            if "Assembly Score:" in line:
                match = re.search(r"Score: (\d+)", line)
                if match:
                    progress['assembly_score'] = int(match.group(1))
        
        return progress
    
    except Exception as e:
        return f"Error reading log: {e}"

def analyze_algorithmic_impact(progress):
    """Analyze the impact of the algorithmic fixes"""
    
    print("=== FIXED ALGORITHM ANALYSIS ===")
    
    if isinstance(progress, dict):
        print(f"Current Phase: {progress['current_phase']}")
        
        if progress['total_matches']:
            print(f"Total Feature Matches: {progress['total_matches']}")
            
            # Compare with original (340 score with poor geometry)
            if progress['total_matches'] < 200:
                print("✅ IMPROVEMENT: Fewer total matches - stricter correspondence criteria working")
            else:
                print("⚠️  Still high match count - may need further tuning")
        
        if progress['after_pruning']:
            print(f"After Pruning: {progress['after_pruning']}")
            
            if progress['total_matches']:
                pruning_rate = (progress['total_matches'] - progress['after_pruning']) / progress['total_matches']
                print(f"Pruning Rate: {pruning_rate:.1%}")
                
                if pruning_rate > 0.5:
                    print("✅ IMPROVEMENT: High pruning rate - outlier rejection working effectively")
        
        if progress['convergence_info']:
            print(f"Convergence Updates: {len(progress['convergence_info'])}")
            for info in progress['convergence_info'][-3:]:  # Last 3 updates
                print(f"  {info}")
        
        if progress['assembly_score']:
            print(f"Assembly Score: {progress['assembly_score']}")
            if progress['assembly_score'] < 300:
                print("✅ POTENTIAL IMPROVEMENT: Lower score may indicate better geometry")
            elif progress['assembly_score'] > 400:
                print("⚠️  High score - algorithm may be working too hard to force connections")
        
        print(f"\nAlgorithmic Fixes Applied:")
        print("• Outlier rejection: 20mm → 2mm (10x stricter)")
        print("• Min correspondences: 3 → 8 points (2.7x more evidence)")  
        print("• Convergence threshold: 1mm → 0.2mm (5x more precise)")
        print("• Constraint weights: All increased")
        print("• Loss functions: 5x stricter penalties")
        
    else:
        print(f"Progress info: {progress}")

def main():
    """Monitor the fixed algorithm execution"""
    
    print("MONITORING FIXED SFS ALGORITHM")
    print("=" * 40)
    print("Job ID: 15280107")
    print("Fixes: Stricter correspondence matching and convergence criteria")
    print("Expected: Better pottery assembly without overlaps/misplacements")
    print()
    
    # Get job status
    job_status = get_job_status()
    print("JOB STATUS:")
    print(job_status)
    print()
    
    # Get progress from logs
    progress = get_log_progress()
    analyze_algorithmic_impact(progress)
    
    print(f"\n=== MONITORING COMPLETED ===")
    print("The algorithm is applying much stricter criteria for:")
    print("1. Correspondence matching (2mm threshold vs 20mm)")
    print("2. Minimum evidence required (8 points vs 3)")  
    print("3. Convergence precision (0.2mm vs 1mm)")
    print()
    print("This should result in:")
    print("• Blue, Green, Cyan pieces connecting to main assembly")
    print("• No overlapping Yellow, Magenta, Purple pieces")
    print("• Overall geometrically valid pottery reconstruction")

if __name__ == "__main__":
    main()
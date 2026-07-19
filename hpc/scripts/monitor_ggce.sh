#!/bin/bash

# GGCE Real-Time Monitoring Script
# Monitors running SLURM jobs for GGCE activity and segmentation faults

echo "=== GGCE Real-Time Monitor ==="
echo "Monitoring jobs: 15696875, 15697164"
echo "Watching for:"
echo "  - GGCE debug output"
echo "  - Segmentation faults"
echo "  - Connectivity improvements"
echo "  - Graph building progress"
echo ""

# Function to check a specific job
check_job() {
    local jobid=$1
    local logfile="slurm-${jobid}.out"

    echo "=== Job $jobid Status ==="

    if [ -f "$logfile" ]; then
        echo "Log file exists: $logfile"
        local lines=$(wc -l < "$logfile")
        echo "Current log lines: $lines"

        # Check for segfaults
        if grep -q "Segmentation fault\|segfault\|core dumped" "$logfile" 2>/dev/null; then
            echo "🚨 SEGMENTATION FAULT DETECTED!"
            grep -n "Segmentation fault\|segfault\|core dumped" "$logfile"
        fi

        # Check for GGCE activity
        if grep -q "GGCE:" "$logfile" 2>/dev/null; then
            echo "✅ GGCE activity detected:"
            grep "GGCE:" "$logfile" | tail -3
        fi

        # Check connectivity status
        if grep -q "Graph.*pieces" "$logfile" 2>/dev/null; then
            echo "📊 Graph status:"
            grep "Graph.*pieces" "$logfile" | tail -3
        fi

        # Show last few lines for progress
        echo "📝 Latest output:"
        tail -5 "$logfile"

    else
        echo "⏳ Log file not yet available"
    fi

    echo ""
}

# Monitor both jobs
while true; do
    echo "$(date): Checking job status..."

    # Check if jobs are still running
    running_jobs=$(squeue -u $USER -h -o "%i" | grep -E "(15696875|15697164)" | wc -l)

    if [ $running_jobs -eq 0 ]; then
        echo "🏁 All monitored jobs completed"
        break
    else
        echo "🔄 $running_jobs jobs still running"
    fi

    check_job 15696875
    check_job 15697164

    echo "==============================================="
    echo "Waiting 30 seconds before next check..."
    sleep 30
    echo ""
done

echo "=== Monitoring Complete ==="
echo "Final job status:"
sacct -u $USER --format=JobID,JobName,State,ExitCode,End | grep -E "(15696875|15697164)"
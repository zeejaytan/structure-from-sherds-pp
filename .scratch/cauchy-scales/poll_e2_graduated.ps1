# Laptop-side poll for cauchy-scales ticket 02, Slurm job (E-2 graduated schedule).
# WHY: scripts/slurm_poll.sh's `set -o pipefail` fails in this host's WSL bash and
# a plain `bash` poll has no Windows SSH key. This runs over Windows OpenSSH from
# PowerShell and never concludes on a transient ssh failure -- only when the job
# really leaves the queue (squeue empty with exit 0, or "Invalid job id").
# Output: .scratch/cauchy-scales/poll-<jobid>.txt (the lead reads this).
param(
    [string]$JobId = '',
    [int]$MaxMinutes = 150
)
if ([string]::IsNullOrWhiteSpace($JobId)) {
    Write-Error "Usage: poll_e2_graduated.ps1 -JobId <slurm job id>"
    exit 2
}
$ErrorActionPreference = 'Stop'
$ssh = 'C:\Windows\System32\OpenSSH\ssh.exe'
$out = Join-Path $PSScriptRoot "poll-$JobId.txt"
$joblog = "/data/gpfs/projects/punim2657/sfs_main/slurm-$JobId.out"
$runarea = '/data/gpfs/projects/punim2657/sfs_main'
$buildlog = "/data/gpfs/projects/punim2657/sfs_main/sfspreproc-docker/build_sif/e2_build_$JobId.log"
$runlog = "$runarea/e2e_pota_e2_$JobId.log"
$dumpdir = '/data/gpfs/projects/punim2657/sfs_main/e2_dumps'

function Invoke-SSH([string]$cmd) {
    try {
        $raw = & $ssh -o ConnectTimeout=20 -o ServerAliveInterval=10 spartan $cmd 2>&1
        return "OK`n" + ($raw -join "`n")
    } catch {
        return "SSHFAIL " + ($_ | Out-String)
    }
}

$lines = New-Object System.Collections.Generic.List[string]
function Say([string]$s) { $lines.Add($s); $s }
function Flush { Set-Content -Path $out -Value $lines -Encoding utf8 }

Say "poll start $(Get-Date -Format u) job=$JobId (E-2 graduated scale schedule)"
$deadline = (Get-Date).AddMinutes($MaxMinutes)
$state = 'unknown'
$sshFails = 0

while ((Get-Date) -lt $deadline) {
    $r = Invoke-SSH "squeue -j $JobId -h -o '%T %M %R'"
    if ($r -like 'SSHFAIL*') {
        $sshFails++
        if ($sshFails % 5 -eq 0) { Say "[$(Get-Date -Format HH:mm)] ssh failure #$sshFails (not concluding)"; Flush }
        Start-Sleep -Seconds 30
        continue
    }
    $body = ($r -split "`n" | Where-Object { $_ -ne 'OK' }) -join ' '
    $body = $body.Trim()
    if ([string]::IsNullOrWhiteSpace($body)) {
        Say "LEFT QUEUE at $(Get-Date -Format u)"
        break
    }
    if ($body -match 'Invalid job id') { Say "PURGED (=DONE) at $(Get-Date -Format u)"; break }
    if ($body -match 'RUNNING|CONFIGURING|RESIZING|SUSPENDED') {
        $state = $body
        Say "[$(Get-Date -Format HH:mm)] $body"
    }
    elseif ($body -match 'PENDING') {
        $state = $body
        Say "[$(Get-Date -Format HH:mm)] $body (node often full; do NOT resubmit)"
    }
    else { $state = $body; Say "[$(Get-Date -Format HH:mm)] $body" }
    Flush
    Start-Sleep -Seconds 45
}

Say "state at loop exit: $state"
Say "=== sacct ==="
Say (Invoke-SSH "sacct -j $JobId --format=JobID,JobName%20,State,ExitCode,Elapsed,NodeList%16 -X")
Say "=== slurm job log tail -n 120 ==="
Say (Invoke-SSH "tail -n 120 $joblog 2>/dev/null || echo '(no slurm stdout)'")
Say "=== arm run log size ==="
Say (Invoke-SSH "ls -lh $runlog 2>/dev/null || echo '(no arm run log yet)'")
Say "=== build log tail -n 20 ==="
Say (Invoke-SSH "tail -n 20 $buildlog 2>/dev/null || echo '(none yet)'")

# The arm's own printout is the authoritative read; pull its key sections.
Say "=== OUTCOME LINES (control: Sherd 2/8 25.000%, Edge 1/15 6.667%, PROFILE 783, DIVERGED-SOLVE 0, best 7/8 score 262.000) ==="
Say (Invoke-SSH "grep -E 'Sherd accuracy|Shard accuracy|Edge accuracy|Best assembly' $runlog 2>/dev/null | tail -n 6 || echo '(none yet)'")
Say "=== PROFILE / DIVERGED-SOLVE counts ==="
# Here-string, not a double-quoted string: the command substitutions must reach bash
# verbatim, and a double-quoted PS string would interpret $(...) itself.
$tmpl = @'
echo PROFILE=$(grep -cE 'PROFILE' __RUNLOG__ 2>/dev/null || echo 0); echo DIVERGED=$(grep -cE 'DIVERGED-SOLVE' __RUNLOG__ 2>/dev/null || echo 0)
'@
Say (Invoke-SSH $tmpl.Replace('__RUNLOG__', $runlog))
Say "=== ground-truth pair survival (control: SURVIVED 14, FEATURE_MATCHING 1) ==="
Say (Invoke-SSH "grep -E '^  (SURVIVED|FEATURE_MATCHING):' $runlog 2>/dev/null | tail -n 4 || echo '(none yet)'")
Say "=== pair 1-4 state (control: 1-4 SURVIVED) ==="
Say (Invoke-SSH "grep -E 'Pieces 1-4 @' $runlog 2>/dev/null | tail -n 2 || echo '(none yet)'")

# Primary read comes from the dump, which the arm summarises itself; this is the
# independent check that the dump exists and the schedule's phases are in it.
Say "=== dump census ==="
Say (Invoke-SSH "ls $dumpdir/ 2>/dev/null | wc -l; ls $dumpdir/ 2>/dev/null | head -n 5")
Say "=== arm's PRIMARY READ block ==="
# Built with a here-string: the awk program's own single quotes must reach bash
# untouched, and a double-quoted PS string cannot hold them.
$tmpl = @'
awk '/=== PRIMARY READ/{f=1} f{print} /=== secondary/{f=0}' __JOBLOG__ 2>/dev/null | head -n 40 || echo '(arm printout not in job log yet -- read the run log)'
'@
Say (Invoke-SSH $tmpl.Replace('__JOBLOG__', $joblog))
Say "=== independent check: a actually used at iter 0, Icp only (must be 260) ==="
# Built with a here-string so no PowerShell interpolation can touch the awk
# program's own quotes and $ fields: the arm reported the dump path itself,
# but a lazy poll that greps the wrong column lies about the schedule.
$tmpl = @'
cat __DUMPDIR__/dump_e0_*.tsv 2>/dev/null | grep -v '^#' | awk -F'\t' '$1=="Icp" && $2==0 {print $4"\t"$6}' | sort | uniq -c || echo '(no dump yet)'
'@
Say (Invoke-SSH $tmpl.Replace('__DUMPDIR__', $dumpdir))
Say "poll end $(Get-Date -Format u)"
Flush
"done"

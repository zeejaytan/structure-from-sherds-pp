#!/usr/bin/env bash
set -euo pipefail
D=C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330
RUN=/data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips
ssh spartan "scancel 34730091; sleep 2; sacct -j 34730091 --format=JobID,State,ExitCode -P"
B=$(base64 -w0 "$D/t330_audit.cpp")
ssh spartan "echo $B | base64 -d > $RUN/tools/t330_audit.cpp && md5sum $RUN/tools/t330_audit.cpp"
cd "$D/.." >/dev/null

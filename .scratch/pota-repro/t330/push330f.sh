#!/usr/bin/env bash
set -euo pipefail
D="C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330"
ssh spartan "scancel 34730267 34732537 34740219 || true; sleep 2; squeue -u zhuojiat -o '%.9i %.6C %.9T' | head -6"
ssh spartan "mkdir -p /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-f/logs /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-f/out /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-f/bin && cp -a /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-c/tools /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-f/tools && cp -a /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-c/inc_E /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-f/inc_E && cp -a /data/gpfs/projects/punim2657/SfSpp/runs/20261110-2248_6925cb0_t326-potE/work/build /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-f/build"
B=$(base64 -w0 "$D/submit330f.sbatch")
ssh spartan "echo $B | base64 -d > /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-f/tools/submit330f.sbatch"
for i in 1 2 3 4; do ssh spartan "cd /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-f && sbatch --parsable tools/submit330f.sbatch $i"; done
ssh spartan 'sleep 20; squeue -u zhuojiat -o "%.9i %.14j %.6C %.9T %.12l" | grep -E "JOBID|t330"; date'

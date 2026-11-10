#!/usr/bin/env bash
set -euo pipefail
D="C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330"
ssh spartan "mkdir -p /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-h/logs /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-h/out /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-h/bin && cp -a /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-c/tools /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-h/tools && cp -a /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-c/inc_E /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-h/inc_E && cp -a /data/gpfs/projects/punim2657/SfSpp/runs/20261110-2248_6925cb0_t326-potE/work/build /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-h/build"
B=$(base64 -w0 "$D/submit330h.sbatch")
ssh spartan "echo $B | base64 -d > /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-h/tools/submit330h.sbatch && cd /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-h && sbatch --parsable tools/submit330h.sbatch"
ssh spartan 'sleep 30; squeue -u zhuojiat -o "%.9i %.14j %.10P %.6C %.9T %.14R" | grep -E "JOBID|t330"; date'

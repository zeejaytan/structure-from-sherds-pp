#!/usr/bin/env bash
set -euo pipefail
D="C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330"
ssh spartan "mkdir -p /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-g/logs /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-g/out /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-g/bin && cp -a /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-c/tools /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-g/tools && cp -a /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-c/inc_E /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-g/inc_E && cp -a /data/gpfs/projects/punim2657/SfSpp/runs/20261110-2248_6925cb0_t326-potE/work/build /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-g/build"
B=$(base64 -w0 "$D/submit330g.sbatch")
ssh spartan "echo $B | base64 -d > /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-g/tools/submit330g.sbatch && cd /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-g && sbatch --parsable tools/submit330g.sbatch"
ssh spartan 'sleep 25; squeue -u zhuojiat -o "%.9i %.14j %.10P %.6C %.9T %.16R" | grep -E "JOBID|t330"; date'

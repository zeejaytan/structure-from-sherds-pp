#!/usr/bin/env bash
set -euo pipefail
D="C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330"
ssh spartan "mkdir -p /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-e/logs /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-e/out /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-e/bin && cp -a /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-c/tools /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-e/tools && cp -a /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-c/inc_E /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-e/inc_E && cp -a /data/gpfs/projects/punim2657/SfSpp/runs/20261110-2248_6925cb0_t326-potE/work/build /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-e/build"
B=$(base64 -w0 "$D/submit330e.sbatch")
ssh spartan "echo $B | base64 -d > /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-e/tools/submit330e.sbatch && cd /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-e && sbatch --parsable tools/submit330e.sbatch"
ssh spartan 'sleep 20; squeue -u zhuojiat -o "%.9i %.14j %.10P %.6C %.9T" | head -8; date'

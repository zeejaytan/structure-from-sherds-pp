#!/usr/bin/env bash
set -euo pipefail
D="C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330"
R=/data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips
C=$R-c
ssh spartan "mkdir -p $C/logs $C/out $C/bin && cp -a $R/tools $C/tools && cp -a $R/inc_E $C/inc_E && cp -a /data/gpfs/projects/punim2657/SfSpp/runs/20261110-2248_6925cb0_t326-potE/work/build $C/build && ls $C && ls $C/build | head -5 && du -sh $C/build"
B=$(base64 -w0 "$D/submit330c.sbatch")
ssh spartan "echo $B | base64 -d > $C/tools/submit330c.sbatch && cd $C && sbatch --parsable tools/submit330c.sbatch"

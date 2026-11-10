#!/usr/bin/env bash
set -euo pipefail
cd "C:/PR/worktrees/structure-from-sherds-pp/330"
rm -f mor1_inputs.tgz
D=C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330
RUN=/data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips
T326=/data/gpfs/projects/punim2657/SfSpp/runs/20261110-2248_6925cb0_t326-potE
ssh spartan "mkdir -p $RUN/tools $RUN/logs $RUN/out $RUN/bin && cp -a $T326/work/inc_E $RUN/inc_E && ls $RUN"
for f in t330_audit.cpp t330_audit.sh submit330.sbatch; do
  B=$(base64 -w0 "$D/$f")
  ssh spartan "echo $B | base64 -d > $RUN/tools/$f"
done
ssh spartan "chmod +x $RUN/tools/t330_audit.sh; md5sum $RUN/tools/*; echo ---; ls -la $RUN"

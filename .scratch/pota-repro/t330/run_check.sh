#!/usr/bin/env bash
set -euo pipefail
D="C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330"
B=$(base64 -w0 "$D/t330_check.sh")
ssh spartan "echo $B | base64 -d > /data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-c/tools/t330_check.sh"
ssh spartan 'SIF=/data/gpfs/projects/punim2657/sfs_main/sfs_pristine_assembly_326.sif; APT=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer; timeout 400 $APT exec --writable-tmpfs --bind '"/data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips-c"':/panel $SIF bash /panel/tools/t330_check.sh 2>&1 | tail -12; date'

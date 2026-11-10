#!/usr/bin/env bash
set -euo pipefail
D=C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330/data
mkdir -p "$D"
cd "$D"
R=/data/gpfs/projects/punim2657/SfSpp/runs
T=$R/20261110-2248_6925cb0_t326-potE
P=$R/20261110-0740_d628fc0_t323-potE
ssh spartan "tar czf - -C $T/trees/E_MOR1/E/MO Breaklines Axes Ground_Truth_ 2>/dev/null || tar czf - -C $T/trees/E_MOR1/E/MO Breaklines Axes 'Ground Truth'" > mor1_inputs.tgz
ls -la mor1_inputs.tgz
ssh spartan "tar czf - -C $P/out/E_MOR1 ." > t323_mor1_out.tgz
ssh spartan "tar czf - -C $T/out/E_MOR1 ." > t326_mor1_out.tgz
ssh spartan "tar czf - -C $P/out/E_K3R1 ." > t323_k3r1_out.tgz
ssh spartan "tar czf - -C $T/out/E_K3R1 ." > t326_k3r1_out.tgz
ls -la *.tgz

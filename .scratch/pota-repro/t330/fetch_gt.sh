#!/usr/bin/env bash
set -uo pipefail
cd C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330/data
T=/data/gpfs/projects/punim2657/SfSpp/runs/20261110-2248_6925cb0_t326-potE
ssh spartan "cd $T/trees/E_MOR1/E/MO && tar czf - 'Ground Truth'" > gt_mor1.tgz
ls -la gt_mor1.tgz
mkdir -p x_gt && tar xzf gt_mor1.tgz -C x_gt
ls x_gt/"Ground Truth" | head -5; ls x_gt/"Ground Truth" | wc -l
echo "--- pcd head"
head -12 x_mor1_inputs/Breaklines/Pot_E_Piece_01_Breakline_0.pcd
echo "--- T file t323"
cat x_t323_mor1_out/T_0to_0.txt
echo "--- acc t323/t326 MOR1"
cat "x_t323_mor1_out/1. Acc.txt"; cat "x_t326_mor1_out/1. Acc.txt"
echo "--- counts"
ls x_t323_mor1_out | wc -l; ls x_t326_mor1_out | wc -l; ls x_t326_k3r1_out | wc -l

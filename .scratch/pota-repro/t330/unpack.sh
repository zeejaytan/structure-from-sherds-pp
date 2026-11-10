#!/usr/bin/env bash
set -euo pipefail
cd C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330/data
for t in mor1_inputs t323_mor1_out t326_mor1_out t323_k3r1_out t326_k3r1_out; do
  mkdir -p x_$t
  tar xzf $t.tgz -C x_$t
done
echo "--- inputs tree"
find x_mor1_inputs -maxdepth 2 -type d | head
ls x_mor1_inputs/Breaklines 2>/dev/null | head -3
ls x_mor1_inputs/Breaklines | wc -l
ls "x_mor1_inputs/Ground Truth" | wc -l
echo "--- head breakline pcd"
head -12 x_mor1_inputs/Breaklines/Pot_E_Piece_01_Breakline_0.pcd
echo "--- one T file (t323)"
cat x_t323_mor1_out/T_0to_0.txt
echo "--- t323 MOR1 acc"
cat "x_t323_mor1_out/1. Acc.txt"
echo "--- t326 MOR1 acc"
cat "x_t326_mor1_out/1. Acc.txt"
echo "--- file counts"
ls x_t323_mor1_out | wc -l; ls x_t326_mor1_out | wc -l

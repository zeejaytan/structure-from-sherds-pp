#!/bin/bash
#SBATCH --job-name=t329ax
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=24
#SBATCH --mem=48G
#SBATCH --time=00:40:00
#
# Ticket 329 bar 1 step 1: regenerate the 172 axes with the exact t328 job-E recipe
# (run_benchmark_172.py --cap 6 --seeds 10 --step 10, 24 jobs, run from the t322 impl
# tree whose sfs_axes produced axes322/axes323; then t323's build_axes323.py tau 1.2
# kmax 3) and cmp byte-for-byte against t323-axes/out/axes323. ANY differing file
# stops the ticket.
# Note on "328's FINAL binaries": run_benchmark_172.py provably references no binary
# (0 hits for subprocess/Popen/os.system/bin-), so the trim5 final binaries cannot
# enter this computation by construction; their md5s are re-measured and recorded in
# out/trim5_binaries.md5 so the tie-in is on the record rather than asserted.
set -uo pipefail
: "${RUNID:?}"
RUNS=/data/gpfs/projects/punim2657/SfSpp/runs
RUN=$RUNS/$RUNID
T322=$RUNS/20261110-0415_d628fc0_t322-axes
T323=$RUNS/20261110-0740_d628fc0_t323-axes
TRIM5=$RUNS/20261110-2325_123f2ca_t328-trim5
PY=/data/gpfs/projects/punim2657/TORA/envs/tora/bin/python3
DS=/data/gpfs/projects/punim2657/Dataset
CORP=/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp
cd "$RUN" || exit 1
mkdir -p out logs
fail() { echo "[JOB] FATAL $*"; echo "$*" > RUN-FAILED.txt; exit 1; }
corpus_list() {
  local out=$1
  { echo "## $DS"; ls -laR "$DS" | head -500
    echo "## $CORP"; ls -la "$CORP"; ls -la "$CORP/Point" "$CORP/Mesh" "$CORP/Axes" 2>/dev/null | head -300; } > "$out"
}
corpus_list "$RUN/list_corpus_before.txt"
echo "[JOB] $(basename $RUN) job=${SLURM_JOB_ID:-na} start=$(date -u +%FT%TZ)"
# final trim5 binaries: recorded, not inputs (see header)
{ md5sum "$TRIM5/bin-MeshPreprocessingHeadless" "$TRIM5/bin-EdgeLineExtractionHeadless"; cat "$TRIM5/binaries.md5"; } > "$RUN/out/trim5_binaries.md5" 2>&1 || fail "trim5 binaries unreadable"
grep -c "subprocess\|Popen\|os.system" "$T322/impl/run_benchmark_172.py" > "$RUN/out/benchmark_binary_refs.txt" 2>&1 || true
cd "$T322/impl" || fail "no impl dir"
md5sum run_benchmark_172.py sfs_axes/*.py > "$RUN/out/axes329_code.md5"
"$PY" run_benchmark_172.py --out "$RUN/out/axes329" --jobs 24 --cap 6 --seeds 10 --step 10 > "$RUN/logs/axes329.log" 2>&1
echo "[JOB] benchmark rc=$? files=$(ls "$RUN/out/axes329" | wc -l) lines=$(cat "$RUN/out/axes329"/*.xyz | wc -l)"
tail -4 "$RUN/logs/axes329.log"
( cd "$RUN/out/axes329" && md5sum *.xyz ) > "$RUN/out/axes329.md5"
diff -rq "$T322/out/axes322" "$RUN/out/axes329" > "$RUN/out/axes329_vs_axes322.diff" 2>&1
echo "[JOB] vs axes322: diff lines=$(wc -l < "$RUN/out/axes329_vs_axes322.diff") files_same=$([ -s "$RUN/out/axes329_vs_axes322.diff" ] && echo no || echo yes)"
"$PY" "$T323/work/build_axes323.py" "$RUN/out/axes329" "$T322/out/profile_rank.json" "$RUN/out/axes329f" 1.2 3 > "$RUN/logs/axes329f.log" 2>&1
echo "[JOB] filter rc=$? files=$(ls "$RUN/out/axes329f" | wc -l) lines=$(cat "$RUN/out/axes329f"/*.xyz | wc -l)"
( cd "$RUN/out/axes329f" && md5sum *.xyz ) > "$RUN/out/axes329f.md5"
diff -rq "$T323/out/axes323" "$RUN/out/axes329f" > "$RUN/out/axes329f_vs_axes323.diff" 2>&1
GATE=$(wc -l < "$RUN/out/axes329f_vs_axes323.diff")
echo "[JOB] vs axes323: diff lines=$GATE files_same=$([ -s "$RUN/out/axes329f_vs_axes323.diff" ] && echo no || echo yes)"
if [ "$GATE" -eq 0 ]; then echo "GATE-PASS axes329f identical to axes323 (172 files)"; else echo "GATE-FAIL axes329f differs from axes323"; cat "$RUN/out/axes329f_vs_axes323.diff" | head -20; fi
corpus_list "$RUN/list_corpus_after.txt"
diff "$RUN/list_corpus_before.txt" "$RUN/list_corpus_after.txt" > "$RUN/list_corpus.diff" 2>&1
echo "[JOB] corpus listing diff lines: $(wc -l < "$RUN/list_corpus.diff")"
echo "[JOB] complete $(date -u +%FT%TZ)"

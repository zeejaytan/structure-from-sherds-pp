#!/bin/bash
# Pristine Pot A control E2E (01-pristine-pot-a-run): ONE full assembly k=5/b=3
# (upstream main.cpp:29-30) on original_samples/SfS_pp Pot_A_*,
# bound to /Dataset/SfS_pp per class/data_path.h POT_A block (POT_A #defined,
# path=/Dataset/SfS_pp/). O4 Rank 2: INFRA batch shim
# (hpc/tools/main_pristine_batch.cpp) compiled out-of-source against the
# untouched pinned tree — Rank 1 (Xvfb, zero change) proven infeasible by diag
# job 32976970 (segfault 139/134/139 on three GL variants). No Xvfb needed.
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=06:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --job-name=pristine_pota

set -uo pipefail

APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
MAIN=/data/gpfs/projects/punim2657/sfs_main
SIF="${MAIN}/sfs_pristine_assembly.sif"
DATASET="${MAIN}/original_samples/SfS_pp"
RUNDIR="${MAIN}/pristine_pota_${SLURM_JOB_ID}"
SHIM=/workspace/sfspreproc-docker/hpc/tools/main_pristine_batch.cpp

echo "=== PRISTINE POT A E2E (batch shim) ==="
echo "Job ${SLURM_JOB_ID}, $(date), node $(hostname)"
mkdir -p "${RUNDIR}"
echo "--- sif:"; ls -lh "${SIF}"
echo "--- shim:"; ls -l "${MAIN}/sfspreproc-docker/hpc/tools/main_pristine_batch.cpp"
md5sum "${MAIN}/sfspreproc-docker/hpc/tools/main_pristine_batch.cpp"
echo "--- pinned tree untouched:"
git -C "${MAIN}/archive_2025_sfs_modified_src" rev-parse HEAD
git -C "${MAIN}/archive_2025_sfs_modified_src" status --porcelain
echo "--- input census (host side):"
for s in Breaklines Mesh Axes Surfaces "Ground Truth"; do
    echo "    $s Pot_A: $(ls "${DATASET}/$s" | grep -c -i pot_a) files"
done

time "${APPTAINER}" exec \
    --bind "${MAIN}:/workspace" \
    --bind "${DATASET}:/Dataset/SfS_pp" \
    "${SIF}" \
    bash -c '
set -euo pipefail
RUNDIR="/workspace/pristine_pota_'"${SLURM_JOB_ID}"'"
SHIM="'"${SHIM}"'"
echo "=== compile shim out-of-source ==="
mkdir -p /tmp/pb-build && cd /tmp/pb-build
cmake /opt/sfs-pristine -DCMAKE_BUILD_TYPE=Release >"${RUNDIR}/shim_cmake.log" 2>&1
make -j8 >"${RUNDIR}/shim_make.log" 2>&1
echo "pristine objects built"
D="CMakeFiles/Hierarchy-Clear.dir"
cat $D/flags.make >"${RUNDIR}/shim_flags.log" 2>&1 || true
# drop empty GOOGLE_GLOG_DLL_DECL="" tokens: harmless DLL no-ops on Linux that
# break unquoted shell expansion (<command-line>: language string errors)
DEFS=$(grep "^CXX_DEFINES" $D/flags.make | cut -d= -f2- | tr " " "\n" | grep -v "GOOGLE_GLOG_DLL_DECL" | tr "\n" " " || true)
INCS=$(grep "^CXX_INCLUDES" $D/flags.make | cut -d= -f2- || true)
FLAGS=$(grep "^CXX_FLAGS" $D/flags.make | cut -d= -f2- || true)
g++ ${DEFS} ${INCS} ${FLAGS} -I/opt/sfs-pristine -I/opt/sfs-pristine/class -c "${SHIM}" -o CMakeFiles/shim.o
LINK=$(cat $D/link.txt)
# swap the GUI main object for the shim object, keep every lib/flag identical
LINK=${LINK//CMakeFiles\/Hierarchy-Clear.dir\/main.cpp.o/CMakeFiles\/shim.o}
LINK=${LINK//\/opt\/sfs-pristine\/build\/CMakeFiles\/Hierarchy-Clear.dir\/main.cpp.o/CMakeFiles\/shim.o}
LINK=${LINK/-o Hierarchy-Clear/-o Hierarchy-Batch}
echo "${LINK}" > "${RUNDIR}/shim_link.txt"
eval "${LINK}"
ls -lh Hierarchy-Batch
echo "=== run batch E2E ==="
time ./Hierarchy-Batch 8 >"${RUNDIR}/binary.log" 2>&1
rc=$?
echo "batch binary exit ${rc}"
grep -E "Total number|Sherd accuracy|Edge accuracy|Result save finish|Saving transformation|co-first" "${RUNDIR}/binary.log" | tail -n 10 || true
ls -la --time-style=full-iso /Dataset/SfS_pp/Result/ >"${RUNDIR}/result_ls.log" 2>&1 || true
mkdir -p "${RUNDIR}/Result_snapshot"
cp -pr /Dataset/SfS_pp/Result/. "${RUNDIR}/Result_snapshot/" 2>/dev/null || true
exit ${rc}
' > "${RUNDIR}/sbatch_outer.log" 2>&1
rc=$?
echo "container exit ${rc}"
echo "--- rundir:"; ls -la "${RUNDIR}"
exit ${rc}

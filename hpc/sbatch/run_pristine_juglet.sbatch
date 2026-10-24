#!/bin/bash
# Pristine Juglet run (02-pristine-juglet-run): ONE full assembly k=5/b=3
# (upstream main.cpp:29-30, shim hpc/tools/main_pristine_batch.cpp) on
# sfs_preprocessing/Juglet_Dataset_20260916/SfS_pp, bound to /Dataset/SfS_pp
# per the ticket-01 JUGLET_ORIG block (class/data_path.h:1715-1820 @ 978b96c).
# O4 Rank 2 (same as Pot A control): INFRA batch shim compiled out-of-source
# against the untouched pinned tree — Rank 1 (Xvfb) proven infeasible by diag
# job 32976970. JUGLET_ORIG block is extracted verbatim from origin/main
# (commit 978b96c) and appended to a /tmp COPY of the pristine data_path.h;
# upstream POT_A selection is deactivated in that copy only. The pristine
# tree, the SIF and the repo checkout are never written to.
#SBATCH --account=punim2657
#SBATCH --partition=sapphire
#SBATCH --time=06:00:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --job-name=pristine_juglet

set -uo pipefail

APPTAINER=/apps/easybuild-2022/easybuild/software/Compiler/GCCcore/11.3.0/Apptainer/1.3.3/bin/apptainer
MAIN=/data/gpfs/projects/punim2657/sfs_main
SIF="${MAIN}/sfs_pristine_assembly.sif"
DATASET="/data/gpfs/projects/punim2657/sfs_preprocessing/Juglet_Dataset_20260916/SfS_pp"
RUNDIR="${MAIN}/pristine_juglet_${SLURM_JOB_ID}"
SHIM=/workspace/sfspreproc-docker/hpc/tools/main_pristine_batch.cpp
PINNED_SHA=9195c0167e2077a564bd7c91156063252feeb506
BLOCK_SHA=978b96c6e72a2747a5b0eee31e80e2302db0141b
BLOCK_MD5=401195eae5a41579563fc237a9843298

echo "=== PRISTINE JUGLET E2E (batch shim, JUGLET_ORIG) ==="
echo "Job ${SLURM_JOB_ID}, $(date), node $(hostname)"
mkdir -p "${RUNDIR}"
echo "--- sif:"; ls -lh "${SIF}"
echo "--- shim:"; ls -l "${MAIN}/sfspreproc-docker/hpc/tools/main_pristine_batch.cpp"
md5sum "${MAIN}/sfspreproc-docker/hpc/tools/main_pristine_batch.cpp"
echo "--- pinned tree untouched:"
git -C "${MAIN}/archive_2025_sfs_modified_src" rev-parse HEAD
git -C "${MAIN}/archive_2025_sfs_modified_src" status --porcelain
echo "--- origin/main for block extract:"
git -C "${MAIN}/sfspreproc-docker" rev-parse origin/main
echo "--- input census (host side):"
echo "    Breakline_0 Juglet .pcd: $(ls "${DATASET}/Breaklines" | grep -c 'Juglet_Piece_[0-9]_Breakline_0.pcd') files"
echo "    Mesh Juglet .obj: $(ls "${DATASET}/Mesh" | grep -c 'Juglet_Piece_[0-9]_Mesh.obj') files"
echo "    Axes Juglet .xyz: $(ls "${DATASET}/Axes" | grep -c 'Juglet_Piece_[0-9]_Axis.xyz') files"
echo "    Surfaces Juglet: $(ls "${DATASET}/Surfaces" | grep -c Juglet) files"
echo "    Ground Truth: $(ls "${DATASET}/Ground Truth" | wc -l) files"
ls "${DATASET}/Ground Truth"

time "${APPTAINER}" exec \
    --bind "${MAIN}:/workspace" \
    --bind "${DATASET}:/Dataset/SfS_pp" \
    "${SIF}" \
    bash -c '
set -euo pipefail
RUNDIR="/workspace/pristine_juglet_'"${SLURM_JOB_ID}"'"
SHIM="'"${SHIM}"'"
echo "=== assemble JUGLET_ORIG header (INFRA, /tmp copy only) ==="
mkdir -p /tmp/pb-inc/class
cp /opt/sfs-pristine/class/data_path.h /tmp/pb-inc/class/data_path.h
echo "--- upstream actives before:"; grep -n "^#define POT_A$\|^#define JUGLET" /tmp/pb-inc/class/data_path.h || true
git -C /workspace/sfspreproc-docker rev-parse origin/main
BLOCK=$(git -C /workspace/sfspreproc-docker show origin/main:class/data_path.h | sed -n "1715,1820p")
echo "${BLOCK}" | md5sum
sed -i "s/^#define POT_A$/#define POT_A_INFRA_OFF_JUGLET/" /tmp/pb-inc/class/data_path.h
printf "%s\n" "${BLOCK}" >> /tmp/pb-inc/class/data_path.h
echo "--- actives after:"; grep -n "^#define POT_A\|^#define JUGLET_ORIG\|^#define POT_A_INFRA" /tmp/pb-inc/class/data_path.h || true
echo "=== compile shim out-of-source ==="
mkdir -p /tmp/pb-build && cd /tmp/pb-build
cmake /opt/sfs-pristine -DCMAKE_BUILD_TYPE=Release >"${RUNDIR}/shim_cmake.log" 2>&1
make -j8 >"${RUNDIR}/shim_make.log" 2>&1
echo "pristine objects built"
D="CMakeFiles/Hierarchy-Clear.dir"
cat $D/flags.make >"${RUNDIR}/shim_flags.log" 2>&1 || true
DEFS=$(grep "^CXX_DEFINES" $D/flags.make | cut -d= -f2- | tr " " "\n" | grep -v "GOOGLE_GLOG_DLL_DECL" | tr "\n" " " || true)
INCS=$(grep "^CXX_INCLUDES" $D/flags.make | cut -d= -f2- || true)
FLAGS=$(grep "^CXX_FLAGS" $D/flags.make | cut -d= -f2- || true)
g++ -I/tmp/pb-inc -DJUGLET_ORIG ${DEFS} ${INCS} ${FLAGS} -I/opt/sfs-pristine -I/opt/sfs-pristine/class -c "${SHIM}" -o CMakeFiles/shim.o
LINK=$(cat $D/link.txt)
LINK=${LINK//CMakeFiles\/Hierarchy-Clear.dir\/main.cpp.o/CMakeFiles\/shim.o}
LINK=${LINK//\/opt\/sfs-pristine\/build\/CMakeFiles\/Hierarchy-Clear.dir\/main.cpp.o/CMakeFiles\/shim.o}
LINK=${LINK/-o Hierarchy-Clear/-o Hierarchy-Batch}
echo "${LINK}" > "${RUNDIR}/shim_link.txt"
eval "${LINK}"
ls -lh Hierarchy-Batch
echo "=== run batch E2E (9 sherds) ==="
mkdir -p /Dataset/SfS_pp/Result
time ./Hierarchy-Batch 9 >"${RUNDIR}/binary.log" 2>&1
rc=$?
echo "batch binary exit ${rc}"
grep -E "Total number|Sherd accuracy|Edge accuracy|Result save finish|Saving transformation|co-first" "${RUNDIR}/binary.log" | tail -n 10 || true
ls -la --time-style=full-iso /Dataset/SfS_pp/Result/ >"${RUNDIR}/result_ls.log" 2>&1 || true
mkdir -p "${RUNDIR}/Result_snapshot"
cp -pr /Dataset/SfS_pp/Result/. "${RUNDIR}/Result_snapshot/" 2>/dev/null || true
rm -rf /Dataset/SfS_pp/Result
echo "bundle Result/ removed; host bundle left as found"
exit ${rc}
' > "${RUNDIR}/sbatch_outer.log" 2>&1
rc=$?
echo "container exit ${rc}"
echo "--- rundir:"; ls -la "${RUNDIR}"
exit ${rc}

set -e
T=/tmp/t332chk; rm -rf $T; mkdir -p $T/asm
tar -xf /tmp/asm_src.tar -C $T/asm
patch -p1 --batch -d $T/asm < /tmp/asm_patch2.diff
md5sum $T/asm/hpc/tools/main_pristine_batch.cpp $T/asm/main.cpp
grep -n "LCS window: \|SFS_LCS_WINDOW" $T/asm/hpc/tools/main_pristine_batch.cpp | head -6
python $T/patch_guard67.py $T/asm/hpc/tools/main_pristine_batch.cpp $T/G.cpp; echo "guard rc=$?"
grep -c "T67-" $T/G.cpp
grep -n "point_.cols() < 50" $T/G.cpp | head -2

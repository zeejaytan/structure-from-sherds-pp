# Pristine Pot A control — spec

**Prior work searched:** GitHub "structure-from-sherds-pp", "Structure-from-Sherds++: Autonomous Reassembly of Unknown Fractured Potteries via Axis-Constrained Pot Configuration"; sj-yoo.info/sfs; Hugging Face "structure-from-sherds-pp"; the arXiv abstract page's code links. Found: https://github.com/SeongJong-Yoo/structure-from-sherds-pp (upstream `main` @ `9195c0167e2077a564bd7c91156063252feeb506`) — plus project page https://sj-yoo.info/sfs/ (cited in paper Fig. 14 caption) -- or: nothing else found.

**Answers:** S1

**Status:** closed 2026-10-15 — control arm executed (gate 15/15, E2E 8/8 + 15/15, witnessed), open items O1–O4 confirmed 2026-10-14, result written into S1. Follow-on preprocessing check (same date, lead-run): untouched upstream preprocessing does NOT regenerate the authors' inputs — see Close section.

## 目标 (S1 能力半臂)

回答 `intent/S1-does-it-connect-anything-real.md` 的能力一半：在与作者原版一致的纯净树上，对单罐 Pot A 跑端到端，看它是否连出真实接缝。Juglet 臂已在 2026-09-26 以 0/10 可连边结算为方法在该材料上失败（非量尺错）；本对照只证明管线在合适材料上能工作，不证明普遍能力。

## 范围

- **纯净树：** `sfs_main` 旁的 `archive_2025_sfs_modified_src`（文档任务书称谓），即上游原版 @ `9195c0167e2077a564bd7c91156063252feeb506` clean（以 `FORK_VS_UPSTREAM.md:15` 钉死的 `upstream/main` SHA 为准；`git status` 必须干净）。
- **样本 Pot A：** `sfs_main/original_samples/SfS_pp` 内 `Pot_A_*`（8 sherds / 15 edges）。对应 `class/data_path.h:1610-1707` 的 `POT_A_ORIG` 块：`SHARD_NUMBER 8`（`data_path.h:1611`），`POTA_ORIG_BASE "/Dataset/SfS_pp/"`（`:1614`），Breaklines/Mesh/Axes/Surfaces×2/Surface_F/GT 8 件 + `Pot_A_simple_graph.txt`（`:1616-1695`），全开（`:1697-1706`）。
- **只综合不采访：** 本 spec 按 `docs/agents/workflow.md` stage 3 只综合已有结论；seams 单独列出待确认（见末节）。

## 纯净度：严格原版 + infra 垫片

- 允许的唯一改动是让原版跑起来的 infra 垫片（数据集选择宏、容器内可见的 `/Dataset` 绑定、构建脚本），零算法行为改动。对应 `FORK_VS_UPSTREAM.md` 的 INFRA 类（dataset selection is config, not algorithm）。
- 禁止：任何 `class/` 算法改动、阈值/权重/求解器改动、预处理产物混入。`main_headless*.cpp` 为 fork 世代文件（`FORK_VS_UPSTREAM.md:59-62`：`d2445d8` 带 `main_headless.cpp` 346 行；`869e696` 带 `main_headless_correct.cpp`），纯净臂不得默认采用；上游 `main.cpp:43` 顶层具 `PCLVisualizer` viewer + `main.cpp:48-54` 键盘回调，属 GUI-only（见 open 项 4）。
- 参照：`hpc/README.md` 约定（版本化源 vs Spartan live 拷贝；结果/日志/数据集/容器/构建目录不上版本）。

## 判定线（对照口径）

- **Gate 探针：** 真值摆位下 join gate 通过率。S1 记录的先例：Pot_A 15/15 通过 vs Juglet 0/18（`intent/S1:11-13`）。纯净臂必须先复现该探针，再谈组装。
- **端到端：** 对照论文 Table V 单罐 Pot A（`A (8)`）：SfS++ SA 100 / EA 100（Table V: `SfS++ (ours) | 100 | ...` SA 行；EA 行 `SfS++ (ours) | 100 | ...`；`k=5,b=3`，见 `papers/text/sfspp-2502.13986v1.md:344-356`）。指标定义见该文 V-B（`τR=20°`、`τt=50mm`，`:374-378`）。
- **87.3% 不是 Pot A 口径：** 87.3% SA 是 10 罐混合 142 碎片 `ALL (142)`（Table VII: `SfS++ (ours) | 80.0 | 96.6 | 97.6 | 92.5 | 87.3`，`:400-407`；正文 `:411`）。不得用作本单罐对照的通过线。
- 通过 = gate 探针复现 + E2E 在 Table V Pot A 口径下报告 SA/EA（含 0 结果的如实报告亦为有效回答）。

## Open 项（必须先关再跑）

1. **`.sif` 构建 recipe 未知：** `hpc/containers/container.def` 只构建 `SfSpp_preprocessing`（`mesh_processing`/`edgeline_extraction`），非组装镜像；组装侧 `.sif` 的 recipe 缺失。纯净构建从哪个 recipe 出镜像待确认。
2. **`/Dataset` bind 映射：** `data_path.h:1614` 写死 `/Dataset/SfS_pp/`，`data_path.h:42` 注“every sbatch binds `<dataset>/SfS_pp` to /Dataset”；`original_samples/SfS_pp` 以哪个宿主路径绑到 `/Dataset/SfS_pp` 待确认（现有 sbatch 多绑 `sfs_preprocessing/...` 或 legacy 路径，无一条指向 original_samples）。
3. **`sfs_legacy_temp` 路径 stale：** `AGENTS.md` 定性其为 tray-000 调试二_clone、“Leave alone”；根目录 `run_*.sbatch`/`rebuild_legacy.sh` 等仍 `cd .../sfs_legacy_temp`。纯净臂不得复用该目录，需独立工作区。
4. **上游 GUI-only `main.cpp` 的 headless 运行策略：** 上游入口依赖 PCL 可视化（`main.cpp:16-17,43-54`）；headless 入口均为 fork 世代新增。纯净对照用哪个入口（原版 + 虚拟显示 / 最小 headless 垫片并声明为 INFRA / 其他）待确认，且不得把 fork 的调参树带入。

## Seams（待确认，不在 spec 内拍板）

- S1：端到端组装分数 + 落盘结果目录（现有 sbatch 以 `--bind "${FRESH}:/Dataset/SfS_pp"` + `Hierarchy-Clear` 为例，如 `hpc/sbatch/run_e2e_pota_fresh.sbatch:46`；纯净臂的 FRESH 指向 original_samples 的具体值待 lead 确认）。
- S2：gate 探针脚本归属（S1 记载的 `gate_probe_b0.py` 在纯净树不存在；探针放 fork 侧只读运行、结果回填，待确认）。
- S3：`pristine-check` 与 `claim-s1-check` 两 teammate 的回报本会话对 spec-writer 不可见（fresh 上下文无历史）；本 spec 仅由落盘文件综合（S1 全文、`FORK_VS_UPSTREAM.md`、paper Table V/VII、`data_path.h:1610-1707`、`build_assembly.sh`、`hpc/README.md`、`main.cpp:1-60`、`hpc/containers/container.def`）。若两份回报中有与此冲突的事实，以 lead 裁定为准——待 lead 确认。
- S4：对照是否需要 `--nodelist` 钉节点（S1 2026-10-05 确立 same-node 规则：跨节点 FP 路径不同，A/B 必须钉同节点；纯净单臂是否钉 `bm065` 待确认）。

## 非目标

- 不碰代码、不 `sbatch`、不改 `intent/`；不重调阈值；不把 Juglet 结论外推为普遍无能力（S1 Gate: Juglet 失败是 scope，非 capability）。

## Close (2026-10-15, lead)

Control arm closed per ticket `issues/01-pristine-pot-a-run.md` worker report
2026-10-14 + conservator witness 2026-10-15 ("baseline verified, it rebuilt
perfectly"): gate probe 15/15, E2E SA 8/8 + EA 15/15 at the Table V gauge
(scoring job 32983523), purity held (upstream @ `9195c01`, INFRA shims only).
Open items O1–O4 were confirmed by the lead 2026-10-14 (see ticket
Preconditions); O4 Rank 1 (Xvfb) proven infeasible on the assembly image
(diag job 32976970, segfaults 139/134/139) — Rank 2 batch-entry shim used
and declared INFRA.

Follow-on, same date (lead-run team tasks task-1–3, evidence in
`C:\PR\SfSpp_preprocessing\artifacts\untouched_pota_verify_20261015/`):
running the **preprocessing** side untouched (upstream DominicoRyu @
`d5ee6865`, fresh headless container image `be24a382…`, zero source shims)
on the authors' own Pot A scans does NOT reproduce the inputs the control
ran on — fresh edge files carry no `#` segment headers so untouched
assembly loads 0 of 8, and fresh surfaces for pieces 2–8 sit +1.8–3.5 mm
too high in Z (piece 1 correct at 0.52 mm); fresh-vs-authors gate 6/15.
The untouched MATLAB axis program is incomplete as released (missing
loader, Windows-only paths, a dimension error on current MATLAB, and a
59-line refinement file existing only in our fork) — axes leg closed as a
documented gap. So the control proves the assembler works on suitable
material *given the authors' prepared inputs*; regenerating those inputs
from scans with the released preprocessing is unverified-to-failing.
Nothing in this close changes code, thresholds, or the Juglet scope verdict.

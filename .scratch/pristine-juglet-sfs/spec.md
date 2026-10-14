# Spec: pristine Juglet run (untouched upstream on real curves)

**Answers:** S3

**Prior work searched:** GitHub "structure-from-sherds-pp", "Structure-from-Sherds++: Autonomous Reassembly of Unknown Fractured Potteries via Axis-Constrained Pot Configuration"; sj-yoo.info/sfs; Hugging Face "structure-from-sherds-pp"; the arXiv abstract page's code links. Found: https://github.com/SeongJong-Yoo/structure-from-sherds-pp (upstream `main` @ `9195c0167e2077a564bd7c91156063252feeb506`) — plus project page https://sj-yoo.info/sfs/ (cited in paper Fig. 14 caption) -- or: nothing else found.

**Goal.** Run the authors' untouched program on the ticket-07 Juglet real-curves bundle
and record whether it finds any joins, answering `intent/S3`. Either outcome settles
something: 0 keeps the S1 cause finding and rules out our modifications; joins found
reopens S1.

**Non-goals.** No preprocessing changes. No algorithm edits (Rank 2 shim only: viewer
display calls removed, nothing else). No new container deps. No Tray-000 work.

**Inputs (verified 2026-10-14, three read-only audits).**
- Bundle: `/data/gpfs/projects/punim2657/sfs_preprocessing/Juglet_Dataset_20260916/SfS_pp`
  — 18 surfaces, 18 breaklines (10.3–26.8 mm), 9 axes (PotSAC 9/9), 9 meshes, GT 10 files.
  Laptop holds fragments only (`artifacts/juglet_input/`, `artifacts/juglet_bl/`,
  `artifacts/juglet_run1/` probes); Surfaces/Axes verification rests on job logs.
- Honest denominator 0/10; non-contacts: 1-5, 1-6, 4-5, 4-6, 5-9, 6-8, 7-8, 8-9.

**Infra reuse (from pristine Pot A control, O1–O4 confirmed 2026-10-14).**
- Recipe `hpc/containers/container-pristine-assembly.def` — no change needed.
- Shim `hpc/tools/main_pristine_batch.cpp` — reuse as-is (k=5/b=3).
- Bind template: `--bind "/data/gpfs/projects/punim2657/sfs_main/original_samples/SfS_pp:/Dataset/SfS_pp"`
  (Pot A). Juglet-side bind TBD in ticket 01.
- Open item: `class/data_path.h` JUGLET block (:1498–1604, SHARD 9) is host-absolute
  (`JUGLET_BASE .../sfs_preprocessing/...`), not `/Dataset`-based — ticket 01 adds a
  JUGLET_ORIG-style block mirroring POT_A_ORIG (:1610–1707).

**Gauges.** Gate probe `artifacts/juglet_run1/gate_probe_b0.py juglet` (Pot_A 15/15
precedent, S1:11-13); Table V A(8) SA100/EA100 k=5/b=3 τR20° τT50mm; honest 0 accepted.

**Tickets.**
- 01 — Juglet-side data block + bind (laptop edit; blocks 02).
- 02 — Pristine Juglet assembly run on Spartan (blocked by 01).
- 03 — Score vs honest 0/10, witness render, verdict into S3 (blocked by 02, Needs-eye).

**Acceptance.** S3's four Done-when boxes checked with job IDs, scores, witnessed render,
and a verdict line. `python scripts/check_intent_links.py` returns 0.

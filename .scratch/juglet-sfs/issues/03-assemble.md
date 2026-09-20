# 03: Assemble the Juglet headless and render the verdict

**What to build:** an SFS++ assembly run over the 9 Juglet pieces and a
witnessed verdict both a conservator and S1 can use: connections found
(count + which pairs), and a render of what the machine claims joins.

**Answers:** S1

**Blocked by:** 02

**Status:** ready-for-agent

## Comments

- 2026-09-20: research before acting (all verified in code, not assumed):
  format checks (axes/breaklines/surfaces/GT all parse; missing
  Surface_F + Breakline_1 inert: graceful loads, guarded use,
  empty-safe tree/correspondence); no behavior branches on dataset
  defines; container :/workspace bind verified for sfspreproc.sif;
  binary is Hierarchy-Clear (Tray script's -headless suffix stale).
  Deviations from the ticket text: REAL GT paths (derived ticket 01,
  after that line was written -- scored assembly serves S1);
  run_juglet.sbatch at repo root (matches 10 existing run scripts).
  JUGLET block committed (absolute bundle paths, SHARD_NUMBER 9);
  active define POT_A -> JUGLET. Submitted as 30829350 (6 h:
  fresh build + 9-piece run); ps1-watched.

**Needs-eye:** viewer bundle TBD at staging time (stage under
`visual-qa/viewer/pairs/juglet_sfs.json`)

- [ ] JUGLET block in `class/data_path.h` (SHARD_NUMBER 9, no GT paths needed —
      missing GT degrades to identity, accuracy section reports 0/0)
- [ ] Headless build + `run_juglet.sbatch` from `sfs_main/`; poll to completion
- [ ] Connection count reported; if still zero, radius-vs-cloud units printed
- [ ] At least one proposed join rendered at a view that shows the break faces
- [ ] Result written back into `intent/S1-does-it-connect-anything-real.md`

# 03: Assemble the Juglet headless and render the verdict

**What to build:** an SFS++ assembly run over the 9 Juglet pieces and a
witnessed verdict both a conservator and S1 can use: connections found
(count + which pairs), and a render of what the machine claims joins.

**Answers:** S1

**Blocked by:** 02

**Status:** ready-for-agent

**Needs-eye:** viewer bundle TBD at staging time (stage under
`visual-qa/viewer/pairs/juglet_sfs.json`)

- [ ] JUGLET block in `class/data_path.h` (SHARD_NUMBER 9, no GT paths needed —
      missing GT degrades to identity, accuracy section reports 0/0)
- [ ] Headless build + `run_juglet.sbatch` from `sfs_main/`; poll to completion
- [ ] Connection count reported; if still zero, radius-vs-cloud units printed
- [ ] At least one proposed join rendered at a view that shows the break faces
- [ ] Result written back into `intent/S1-does-it-connect-anything-real.md`

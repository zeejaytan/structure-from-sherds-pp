# 01: Juglet-side data block + bind

**What to build:** the untouched binary reads the Juglet bundle through a
`/Dataset`-based data block, exactly the way the Pot A control reads its samples —
so the Spartan run serves real Juglet curves with zero algorithm changes.

**Answers:** S3

**Blocked by:** None (can start immediately).

**Status:** ready-for-agent

- [ ] `class/data_path.h` gains a JUGLET_ORIG-style block mirroring POT_A_ORIG (:1610–1707): SHARD_NUMBER 9, `/Dataset`-relative base, 9 breakline_0 paths, 9 meshes, 9 axes, 18 surfaces, 9 T + simple graph (names mirror the existing JUGLET block :1498–1604, base swapped)
- [ ] Existing JUGLET block (:1498–1604) and POT_A_ORIG block untouched; active macro unchanged
- [ ] Juglet-side `--bind` host path recorded here (host dir containing the `SfS_pp` bundle root)
- [ ] Gate probe paths resolve against the new block (dry check: filenames match bundle listing)

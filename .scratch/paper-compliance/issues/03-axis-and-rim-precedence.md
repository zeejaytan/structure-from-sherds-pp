# 03: Trace axis precedence and rim-flag semantics end to end

**Answers:** S1

**Blocked by:** nothing — read-only until the trace says otherwise

**Status:** ready-for-agent

**Needs-eye:** none — a precedence map, not a geometry claim. Renders enter
only if a traced path turns out to depend on geometry nobody has looked at.

## Why this ticket exists

Two things were mapped but explicitly NOT interpreted during the audit,
because both are subtle enough to misread:

1. **Axis precedence.** The mains load axis files (`ReadAxis`, empty file →
   shard dropped). The repo also carries `ComputePottmannAxis`,
   `ComputePotSACAxis`, `RefineAxis` (`class/axis_estimation.h`), called
   from `axis_estimation.cpp:160/185/192` and `ranking_system.cpp:1833`.
   Which one wins where — files seeding computation, computation overriding
   files, or parallel paths that never meet — was not traced.
2. **Rim-flag semantics in matching.** `is_seg_rim_` is read in `BuildTree`
   (`data_structure.cpp:116-139`), both LCS paths
   (`feature_matching.cpp:227/236/339/348`), profile/rank checks, and all
   over `reconstruction.cpp`. What "rim" causes at each site (inclusion,
   exclusion, gating) was cited but not interpreted.

Either can hide a real behavior: a computed axis silently overriding a good
file (or vice versa), or a flag meaning the opposite of what its name
suggests at one call site.

## What to do

1. Trace, for a Pot_A run: axis file → `ReadAxis` → every downstream use,
   marking each site file-driven, computed, or refined-from-file. Same for
   one Juglet run (archive paths). Output is a precedence map: ordered list
   of file:line with one line each saying what flows into what.
2. For each `is_seg_rim_` read site, write one line stating the effect
   (e.g. "excludes rim segments from the LCS tree" — VERIFY, do not copy
   this example). Flag any site where the effect contradicts the flag's
   apparent meaning.
3. Only then decide: anything found becomes a fix ticket with the site and
   the predicted pair movement; a clean trace closes this ticket as the
   map itself (which the next debugger will need).

## Acceptance criteria

- [ ] Precedence map: file → load → compute/refine → consume, per binary
      that runs (at minimum `main_headless_correct`, the one sbatch uses)
- [ ] Per-site rim-flag effect list, each verified by reading the lines
- [ ] Any contradiction filed as its own fix ticket with predicted pair
      movement, or explicitly recorded as understood-and-accepted
- [ ] No code changes in THIS ticket — it is a trace. Code changes it finds
      get new tickets, so the trace stays readable after the fix

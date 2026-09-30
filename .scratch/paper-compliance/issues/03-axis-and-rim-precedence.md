# 03: Trace axis precedence and rim-flag semantics end to end

**Answers:** S1

**Blocked by:** nothing — read-only until the trace says otherwise

**Status:** resolved 2026-09-30 — trace complete; this ticket IS the map.
No code changed in this ticket per its own rule.

**Needs-eye:** none — a precedence map, not a geometry claim. Renders enter
only if a traced path turns out to depend on geometry nobody has looked at.

## Precedence map (`main_headless_correct`, the sbatch binary)

`CMakeLists.txt:157` builds it; all sbatch files launch it. Pot_A vs Juglet
differ only in compile-time path tables (`data_path.h:1417-1426` /
`:1489+`); code path identical. Verified by reading the lines, 2026-09-30:

1. `data_path.h` `axis_path[]` → `main_headless_correct.cpp:171`
   `ReadAxis` → `data_structure.cpp:146-187` fills axis vectors. **File is
   the only axis source in-binary.**
2. `:172-175` empty/unopenable file → shard dropped (`:152-155` early
   return). `:179-183` <50 breakline points → axis cleared + drop.
3. `:270-280` axis height snapshotted; `:290/305` `AxisAlignment`
   (`reconstruction.cpp:69-127`) consumes the file axis (rotate to z,
   translate to origin); `:336` `FeatureComp` uses `axis_point_.size()`
   for axis-pair combinatorics; axis reads at
   `feature_matching.cpp:1815,2003` diagnostic only.
4. All ICP `AxisConsistency` fits read breakline points/normals, never the
   loaded vectors — the file acts only through the aligned frame.
5. `ranking_system.cpp:1831-1832` (`GraphAxisRefinement`) resets the root
   axis to canonical zero before `RefineAxis` — **the file provably does
   not seed refinement** (values restored at `:1078-1079`, delta absorbed
   into `T_`: no corruption, but silent non-seeding — F3, accepted below).
6. `ComputePotSACAxis` never called in-binary; sole caller is the offline
   tool `hpc/tools/edgeline_extraction.cpp:70-77`. Parallel path meeting
   only in the file it writes.

**Verdict: file wins everywhere at runtime.** Nothing computed overrides it.

## Rim-flag effects (all verified, one line each)

- `data_structure.cpp:122-136` (BuildTree): rim segments excluded from the
  kd-tree. Strongest effect. Non-rim: all points inserted.
- `feature_matching.cpp:227,236` + `:339,348` (both LCS paths): rim spans
  forced to DP-zero — excluded from matches. (Names `Rim_aS/Rim_bE` are
  crossed but index pairing verified correct.)
- `feature_matching.cpp:1270,1420` (`CountPCInlier`): rim-height ceiling —
  candidates extending beyond the rim band vetoed.
- `reconstruction.cpp:1235-1237,1459-1460,1653-1658,1765-1766,1876,1945,2073,2134`
  (all ICP variants): rim constraint only with ≥2 / both / merge-and-fixed
  flagged; applies to flagged nodes only.
- `ranking_system.cpp:1916-1927` (`CheckGraphPlausibility`): over-rim-height
  configuration rejected.

**No site contradicts the flag's meaning.** All live reads gate in the
"rim constrains" direction.

## Findings disposition

- F1 (dead rim-only `ExclusivelyPickEdge` branch, zero callers — verified
  2026-09-30: decl + def only): understood-and-accepted. Dies with the
  function if base-flag removal ever takes it.
- F3 (refinement seeds from zero): understood-and-accepted — no corruption
  (values restored, delta in `T_`), file simply doesn't seed. Re-seeding
  would change solves; not this ticket.
- F4 (double kd-insert in debug builds): release builds single-insert;
  noted, not acted on.
- F5 (`MakeRimData` overload asymmetry): harmless while callers pass
  consistent data; noted.
- F2 (`NO_BASE_INFO` doesn't clear `is_sane_base_`): belongs to ticket 02;
  filed there.

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

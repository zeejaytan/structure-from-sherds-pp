# 02: `NO_BASE_INFO` is defined in every main — decide if the base half lives or dies

**Answers:** S1

**Blocked by:** nothing — read code, measure, then decide

**Status:** resolved 2026-09-30 — verdict (b) executed: define completed,
dead reads removed, rebuild clean in-container. No half state remains.

**Needs-eye:** none — compile flags and match counts, no geometry claim.

## Closeout 2026-09-30

- `NO_BASE_INFO` clears `is_sane_base_` too in all three mains (F2 fixed;
  provable no-op: every writer emits flag 0).
- Commented-out base escape (`feature_matching.cpp:2088-2094`) and uncalled
  base-only `ExclusivelyPickEdge` branches removed. Parse site + live
  FeatureComp skips stay for a future true flag source.
- In-container rebuild (`build_sif`) exit 0; new code in the binary.
- Per-pair assembly rerun waived with recorded reason (S1 entry 2026-09-30):
  zero executed instructions change on current data.
- Enabling with a true flag source remains a preprocessing-repo question
  (from `check_base_and_rim.m`), filed there, not here.

## Spike outcome (verified 2026-09-30, all cites re-read by the lead)

- Reader half is NOT inert: `feature_matching.cpp:1616-1621` and
  `:1686-1691` (triple sane/sane-base skips) would genuinely alter
  matching if flags were ever true. Both live (all mains call
  `FeatureComp`; ranking calls graph building at `:835,873`).
- Writer half is unanimous: all three C++ edgeline writers hardcode flag
  `0` (headless `:1367,1371`; legacy `:1083,1087`; curvature-fixed
  `:229`); the one MATLAB path computing `isbase`
  (`check_base_and_rim.m:48-113`) cannot serialize it
  (`*_frag_type` functions undefined). No file can set base flags, so
  un-defining `NO_BASE_INFO` today is a provable no-op.
- Dead under either verdict: commented-out read
  (`feature_matching.cpp:2088-2094`), uncalled `ExclusivelyPickEdge`
  base branches (`ranking_system.cpp:1197-1223`; zero callers verified:
  decl + def only).
- F2 (found by the trace ticket, verified): `main_headless_correct.cpp:202-206`
  clears `is_seg_base_` but NOT `is_sane_base_` (`data_structure.cpp:236`
  sets it). Half-state: sane-base skips stay armed. Dormant on current
  files (writers emit 0 → both flags false) but the define lies about it.

## Decision (verdict b, executed as two changes)

1. **Complete the define**: clear `is_sane_base_` alongside `is_seg_base_`
   in all three mains (`main.cpp:107-111`, `main_headless.cpp:160-163`,
   `main_headless_correct.cpp:202-206`). Provable no-op on current files
   (both flags already false on every emitted breakline) — verified by
   rebuild + Pot_A/Juglet per-pair numbers unchanged.
2. **Remove the dead reads**: commented-out `:2088-2094` gate and the
   uncalled `ExclusivelyPickEdge` idx-2/3 branches. The parse site
   (`data_structure.cpp:234-236`) and the live skips (`:1616,:1686`) STAY —
   they are the machinery a future true flag source would use; deleting
   them would destroy verdict-(b) option value for zero runtime effect.
3. Enabling with a true flag source is NOT this ticket: it needs
   base-flag emission in the preprocessing repo (starting from
   `check_base_and_rim.m`) — filed there if anyone wants it.

## Acceptance criteria (updated 2026-09-30)

## Why this ticket exists

`#define NO_BASE_INFO` sits in `main.cpp:34`, `main_headless.cpp:32`, and
`main_headless_correct.cpp:52` (`NO_RIM_INFO` commented out in all three),
forcing `is_seg_base_=false` in every binary. Downstream, the base-pair
logic — the `sane&&seg_base` skips in `FeatureComp`/`FeatureCompGraphBuilding`,
the base-only paths in `ExclusivelyPickEdge` — never executes. The paper's
rim/base machinery runs here on rim flags alone. Either the base half is
dead weight that should go, or it is a live feature switched off — currently
neither, which is the worst option because the next reader cannot tell.

## Research spike (do first)

1. Read every `is_seg_base_` / `is_sane_base_` read (notably
   `feature_matching.cpp:1616-1691`, `ranking_system.cpp:1177-1223`) and
   write down, per site, what changes if the flag could be true. Distinguish
   "would change matching" from "reads a constant."
2. Check where `is_seg_base_` would even come from: the breakline file's
   segment flags (`data_structure.cpp:234-236`, abs index 2/3) — do our
   emitted files ever carry base flags? (The edgeline writer sets rim flags
   from axis deviation; base-flag emission needs checking, not assuming.)
3. If no emitted file can set the flag AND no reader would act on it: the
   honest fix is removal of the dead paths, not enabling. If some path
   would act: enabling needs the flag to mean something true first, which is
   a breakline-writer question back in the preprocessing repo — file it
   there, don't invent semantics here.

## Acceptance criteria

- [ ] Spike: per-site behavior-with-flag table, plus where base flags could
      originate (file format supports them; does any writer emit them?)
- [ ] Decision recorded: remove dead paths, or enable with a true flag
      source — with the Pot_A per-pair numbers showing nothing moved
      (removal) or the predicted pairs moving (enable)
- [ ] No half state: after this ticket, either no `NO_BASE_INFO` anywhere
      with flags meaning something, or no base-flag code at all
- [ ] Juglet honest-10 unchanged-or-better either way

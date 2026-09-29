# 02: `NO_BASE_INFO` is defined in every main — decide if the base half lives or dies

**Answers:** S1

**Blocked by:** nothing — read code, measure, then decide

**Status:** ready-for-agent

**Needs-eye:** none — compile flags and match counts, no geometry claim.

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

# 08: Correspondence normal gate + grouping angle

## V1 OUTCOME 2026-10-02 (job 32090601): grouping 10° does NOT recover

0/8 + 0/15, 22 states, best 745, profile 87/0 (zero kills), State 0 =
2+1+3+2 fragments (no large graph). Against the 25° arm (2/8, best
567/232, 5–6-piece graphs): tighter grouping fragments merging (max
3-piece vs 5–6-piece) at HIGHER best score (745) with zero correct.
Grouping moves granularity, not correctness; 25° stays (paper value +
only arm that ever placed pieces). Next: normal gate stage×angle (V2),
then ticket closes or continues on numbers.

**Answers:** S1

**Blocked by:** nothing — assembly A/B measurements; one variable each

**Status:** ready-for-agent

**Needs-eye:** none — gate counts and pair movement, no geometry claim.

## Why this ticket exists

Two verified value/structural deviations in match formation (ticket 05
items 6–7):

1. **Normal gate, wrong stage and wrong angle.** Paper line 215 prunes ICP
   correspondences with normal differences > 30°. Code: `MakeCor` has NO
   angle pruning (mutual-nearest only, verified); angle appears only in
   inlier counting at 0.262rad ≈ 15° (`feature_matching.cpp:1079`,
   `reconstruction.h:26`, verified). So the gate is tighter than the
   paper's AND lives a stage later (counting, not correspondence).
2. **Grouping rotation 10° vs 25°.** Paper line 294 (25°/20mm). Code:
   `isSimilar(0.175, 20)` (`feature_matching.cpp:1745`, verified) =
   10.0°/20mm. Translation matches; rotation is 2.5× tighter.

## What to build (one variable each, in this order)

1. Grouping angle 10° → 25° (translation fixed at 20): assembly A/B per
   pair. Prediction to beat: more pairs merge at grouping, plausibility
   decides downstream — record which pairs move and whether any passing
   pair is lost to over-merging.
2. Normal gate: add correspondence-stage pruning at 30° (paper value) and
   separately test relaxing the inlier stage 15° → 30°. These are TWO
   variables (stage × angle) — do not bundle; the audit found them split
   and they must be rejoined deliberately or kept split deliberately.
3. State the end condition per variable: paper value adopted, current
   value kept-with-measurement, or a third value with its own numbers.
   "Restored to ORIGINAL"-style comments without a ticket cite are how
   this drift happened; each surviving value gets a measurement cite.

## Acceptance criteria

- [ ] Grouping angle A/B per pair (Pot_A + Juglet honest + authors'
      sample); no-regression rule
- [ ] Normal gate stage×angle measured separately (up to two A/Bs);
      end condition stated per variable
- [ ] Every surviving value carries a measurement cite in the code comment

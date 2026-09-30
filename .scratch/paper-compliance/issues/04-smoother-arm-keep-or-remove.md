# 04: The SG smoother arm — keep covered, or remove

**Answers:** S1

**Blocked by:** nothing — read code, decide, then a small build/test change

**Status:** ready-for-agent

**Needs-eye:** none — an env switch and numeric coefficients, no geometry claim.

## Why this ticket exists

Ticket 01 settled the smoothing question as cosmetic-for-joins (same 9
pairs both arms, values ±1–2, 0/6 accuracy either way) and closed WITHOUT
implementing — but the SG arm it built to run the comparison is still in
the matching path (`class/filter.cpp:118-158,260-290`, selected by
`SFS_SMOOTHER=sg`, default Lanczos). That is a half-state this repo does
not tolerate (see ticket 02's "no half state" rule): two smoothers ship,
one runs by default, the switch lives only in ticket 01's body, and
nothing proves the dormant arm still builds or runs after later edits to
`CalculateFeatureAxisless`. Either the arm earns its keep or it goes.

This ticket also owns 01's reopen condition ("a future tuned-config run
shows pairs on the margin → re-run A/B there"). That condition belongs to
matcher-tuning work, not to smoothing: restate it there when such a run
exists; do not re-argue the 2.6–20% descriptor gap, which 01 settled.

## Decision inputs (read first, all on record)

- 01's A/B table (same 9 pairs, ±1–2) and verdict; the SG-window caveat
  (paper's window unspecified, 7 used to match — an assumption that must
  be logged wherever the arm survives, never silently adopted).
- The arm's shape: `SavitzkyGolayDiff` (7-pt quadratic first derivative,
  circular) + Gaussian(7, σ=2.0), env-selected, default path byte-identical
  when unset. Thickness handling identical both arms (unfiltered, matches
  the paper).

## What to build

**Option A — keep, covered.** The arm stays iff it is proven, not merely
present:
- Unit test `SavitzkyGolayDiff` against closed-form 7-point quadratic
  first-derivative coefficients (analytic, no fixture needed): exact
  coefficients in, exact derivative out, plus circular wrap on a short
  series. This is cheap and decisive — unlike the 01 comparison, which
  needed full assembly runs.
- Log the assumed window (7) wherever the arm activates (already printed;
  keep it) so a future reader knows what "paper recipe" meant here.
- Both arms build in the normal build (they do — same TU); the test runs
  in whatever harness the repo uses for `ggce_tests`, or documents why it
  cannot.

**Option B — remove.** Delete the SG arm (`SavitzkyGolayDiff`, the
Gaussian-7 overload if SG-only, the env switch and its includes),
restoring single-smoother code. Verify by rebuild + strings check (no
`SFS-SG` in the binary) and default-path numbers unchanged — or waived by
construction with 02-grade rigor (default path untouched byte-for-byte;
state the diff that proves it). If removed, 01's body keeps the A/B table
as the record; a future paper-recipe run re-implements from that spec.

Do NOT do both (keep-but-uncovered is the current state and is the
complaint), and do NOT retune either smoother here — coefficients and
windows are separate decisions with their own measurements.

## Acceptance criteria

- [ ] Decision recorded: keep-covered or remove, with the 2–3 sentence
      reason (cost of carrying an untested matching path vs cost of
      re-implementing a measured comparison)
- [ ] If keep: coefficient test green; assumed window logged; 01's reopen
      condition restated as matcher-tuning's, with the pointer
- [ ] If remove: rebuild clean; binary carries no SG arm; default-path
      numbers unchanged or waived-by-construction with the proving diff
- [ ] No half state afterwards: either one tested smoother switch or one
      smoother — never two paths with one unproven
- [ ] Authors' sample numbers (01's table) cited, not re-run, unless the
      change touches the default path — it must not

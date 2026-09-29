# Spec — paper-compliance closeout (assembly side)

**Answers:** S1

**Status:** spec for slicing; nothing implemented yet

## Problem statement

Ticket 16 (preprocessing repo) audited §IV-B1 and found three gaps living
on this side of the repo boundary: descriptor smoothing differs from the
paper, base-flag machinery is compiled out, and axis precedence was never
traced. Meanwhile this repo computes its own descriptors
(`CalculateFeatureAxisless`), runs its own ranking, and loads axes someone
else estimated. Nothing here is known broken — Pot_A scores 15/15 through
this code — so every ticket below is guilty-until-proven-innocent in the
reverse direction: the burden is on the ticket to show a measurement that
matters, and "the paper says otherwise" alone opens no code.

## Scope decisions (binding, same shape as the preprocessing spec)

1. **Research spike first in every ticket.** Read the cited code, write down
   what it does, and only then propose a change. The sidekick-mapped facts
   in ticket 16's addendum are starting points, each re-checked by reading
   the lines — several were found subtly wrong on re-read already (rim-flag
   semantics uninterpreted; file-vs-computed precedence untraced).
2. **Two lanes, declared up front:** *gate lane* (moves a scored number on
   Pot_A per-pair or the Juglet honest-10) or *compliance lane*
   (no-regression proof + a stated silent-failure reason). Same template as
   `SfSpp_preprocessing/.scratch/paper-compliance/spec.md`.
3. **No duplication across repos.** Preprocessing gaps live there (their
   tickets 02/03/04/05/06/10–16). Tickets here reference them; they do not
   re-specify them. The `Surface_F` question is the shared boundary: ticket
   01 here consumes whatever their ticket 01 emits, and neither side assumes
   the other's schedule.
4. **The authors' Pot_A sample rides along** wherever a "correct" behavior
   is needed, same as the preprocessing chain's ticket-11 rule.

## The gaps and their tickets

| # | gap | ticket |
|---|---|---|
| Descriptor smoothing: Lanczos + Gaussian vs paper's finite-diff + Savitzky-Golay + Gaussian(7, σ=2.0) | 01 here |
| `NO_BASE_INFO` in all mains: base machinery dead | 02 here |
| Axis precedence (files vs ComputePotSACAxis/RefineAxis) + rim-flag semantics in matching | 03 here |

## Out of scope

- MATLAB PotSAC-vs-paper: flagged, needs a MATLAB reader, no owner. Not
  silently dropped — said here.
- Juglet assembly runs: ticket 07 territory (preprocessing repo) once
  breaklines exist; the JUGLET paths already point at the archive bundle.
- Re-tuning matcher thresholds: not paper compliance, not listed. Any
  threshold proposal is a new ticket with its own measurement.

## Done when

- Each ticket resolved (fixed with gate movement, accepted with
  justification, or refuted) and the three rows above stop being gaps.

# S2 — Is the axial-symmetry assumption compatible with our material?

**Status:** open · **Blocked by:** none · **Effort:** a measurement on existing scans

## Why it matters

SfS++ reassembles **axially symmetric** pots: it uses the fact that a wheel-thrown vessel
is a surface of revolution to constrain where a sherd can sit. That assumption is what
makes it work without a trained prior, and it is also its scope limit.

Rabati's Kura-Araxes material is **handmade**, not wheel-thrown. A handmade jar is roughly
symmetric in intent and measurably not symmetric in fact. If the departure is larger than
the tolerance the method allows, SfS++ is not applicable to that part of the assemblage —
and that is a scope statement, not a failure, and must not be reported as one.

The Early Kurgan material may be a different answer from the Kura-Araxes material. Do not
average them.

## Done when

- [ ] Departure from a surface of revolution measured on real scanned sherds, in
      **millimetres** of radial deviation at a given height — a physical number, not a
      fitted score
- [ ] Compared against the tolerance SfS++ actually assumes, read from the paper and the
      code at the pinned version rather than inferred
- [ ] Reported **separately** for wheel-thrown and handmade material
- [ ] A profile drawn — the fitted profile against the measured section — because a
      goodness-of-fit number cannot show *where* the departure is, and where matters

## Gate

If handmade Caucasus material sits outside the assumption, then SfS++ is a baseline for the
wheel-thrown comparison set only. Say that in the comparison table rather than showing it
failing on material it was never meant for — that would be a wrong claim about the method,
which is exactly the distinction this workspace exists to keep straight.

## Source

Upstream: Yoo and Liu et al., *Structure-From-Sherds++*; [`../README.md`](../README.md).
Workspace: `../../intent/U7-what-is-being-compared.md`.

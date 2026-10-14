# S3 — Does the authors' untouched program find any joins on the Juglet?

**Status:** resolved 2026-10-15 — the untouched program agrees: no genuine joins (verdict below). Render staged; conservator witness pending. ·
**Blocked by:** none (was: ticket 01 — landed as 978b96c)

**What this asks.** S1 settled what *our* copy does on the Juglet: an honest **0/10**,
with the cause proven in preprocessing (one wall face traced per sherd, inner-vs-outer
arbitrary, so 10 of 18 answer-key edges are invisible to any 2 mm comparison; the other
8 include 8 non-contacts 3.3–18.9 mm apart). What is still open is whether the
**authors' untouched program agrees** — same real-curves bundle, no modifications of
ours in the path. If it also finds nothing, our modifications are ruled out as the
cause; if it finds joins, our copy diverged from upstream and the S1 story is wrong.

**Why it matters (plain words).** A zero from our own edited copy could always be our
own bug. A zero from the authors' program, on the same sherds, turns "we couldn't make
it join" into "the method as published doesn't join this material" — the finding the
thesis can actually cite. The Pot A control (pristine run 8/8 + 15/15, jobs
32982520/32983523) proves the untouched setup works on suitable material, so a Juglet
zero beside it is a statement about the material, not a broken instrument.

**Untouched means:** upstream `main` @ `9195c0167e2077a564bd7c91156063252feeb506`,
built by `hpc/containers/container-pristine-assembly.def`; the only delta is the Rank 2
shim `hpc/tools/main_pristine_batch.cpp` (viewer's display calls removed so it runs
without a screen — Xvfb proven infeasible, diag job 32976970; no algorithm change,
keeps k=5/b=3 and the graph_dummy quirk). Input is the ticket-07 real-curves bundle:
/data/gpfs/projects/punim2657/sfs_preprocessing/Juglet_Dataset_20260916/SfS_pp
(18 surfaces, 18 breaklines, 9 axes, 9 meshes, GT 10 files; verified 2026-10-14).

**Gauges (both must be reported).**
- Gate probe `artifacts/juglet_run1/gate_probe_b0.py juglet` — must show true mates
  passing at ground truth (Pot_A precedent 15/15). If the gate rejects truth, the run
  cannot propose joins and the zero is explained before assembly starts.
- Paper Table V bar, Pot A column A(8): SA 100 / EA 100 at k=5/b=3
  (`papers/text/sfspp-2502.13986v1.md:344,351,356`), tolerances τR=20° τT=50mm (:378).
  An honest 0 on the Juglet is an accepted outcome, scored against the honest 0/10
  denominator. Note: 87.3% is the 10-pot ALL mix in Table VII (:400–411) — never the bar.

**Non-contact edges (answer key minus physical contacts, gaps 3.3–18.9 mm):**
1-5, 1-6, 4-5, 4-6, 5-9, 6-8, 7-8, 8-9. Honest denominator 0/10.

**2026-10-15 verdict — untouched agrees (0 genuine joins).** Two pristine runs
(jobs 32993790, 32994273, both COMPLETED 0:0, deterministic identical scores):
gate probe STRICT **0/18** at ground truth; assembly SA **2/9**, EA **0/18**.
Attribution is exact (a scorer replica faithful to upstream `CountResult`
reproduces the binary bit-for-bit): the "2 seated" are pieces 1+6 via a single
directed pass 6→1 (12.7° rotation, 39.3 mm shift — inside the 20°/50 mm gate);
the reverse direction fails, and 1-6 is one of the eight NON-contacts above, so
no physical join was found in either direction on any touching pair. Absolute
poses: 0/9 within tolerance (best 35.4°/95.5 mm on different sherds). Reading:
one marginal pass on a non-physical edge, one direction out of 36 trials, with
the gate rejecting truth itself — chance, not a join. Honest score **0/10**.
This is case 1, the method failing on this material — not a broken ruler (Pot A
control 8/8 beside it; GT checksums match) and not a wrong answer key (honest
denominator used). Our modifications are ruled out as the cause of the S1 zero.

**Done when:**
- [x] Gate probe on the Juglet bundle recorded — STRICT 0/18 at GT (2026-10-15)
- [x] Pristine Juglet assembly run recorded — SA 2/9 / EA 0/18, k=5/b=3, jobs 32993790 + 32994273
- [ ] Reassembly rendered and witnessed — staged (`visual-qa/preview/manifest-juglet-pristine.json`, frame_ok, coincidence 2.13 mm); witnessed look + conservator note pending
- [x] Verdict written here — untouched agrees (0 genuine joins, 2026-10-15)

## Where it stands

The untouched program agrees with our copy: no genuine joins on the Juglet (honest
0/10), beside the 8/8 Pot A control from the identical setup. The S1 cause finding
stands, with our modifications ruled out as its source. What remains is the
conservator's look at the staged pair (left: correct juglet; right: the attempt),
then this question's render box is checked.

Weight: one control pot at 8/8 and one test object at honest 0/10 — a clean lead, not a conclusion.
Next: conservator witnesses the staged pair; Tray 000 remains the pot that could show the method working.
Viewer: `manifest-juglet-pristine`

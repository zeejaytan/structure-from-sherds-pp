# S3 — Does the authors' untouched program find any joins on the Juglet?

**Status:** open — verification complete 2026-10-14 (bundle census, infra inventory, gauges pinned); run not yet dispatched. ·
**Blocked by:** `.scratch/pristine-juglet-sfs` ticket 01 (Juglet-side data block + bind)

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

**Done when:**
- [ ] Gate probe on the Juglet bundle recorded (pass count at GT)
- [ ] Pristine Juglet assembly run recorded (SA/EA, k/b, job ID)
- [ ] Reassembly rendered and witnessed (Needs-eye on ticket 03)
- [ ] Verdict written here: untouched agrees (0, cause stands) or diverges (joins found)

## Where it stands

The untouched Pot A control seats 8/8 with 15/15 edges, so the instrument is proven.
The Juglet bundle is censused and the gauges are pinned; what remains is adding the
Juglet-side data block plus bind (ticket 01), running the untouched binary on it
(ticket 02), and scoring plus witnessing the result (ticket 03).

Weight: one control pot at 8/8; the Juglet arm unmeasured — a single-object lead either way, not a conclusion.
Next: ticket 01 (JUGLET_ORIG block + Juglet bind), then the Spartan run.

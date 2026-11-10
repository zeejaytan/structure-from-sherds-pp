
## Lead verification (2026-11-11, independent)

Verified from the artefacts, not from the report: the two assembly arms' own binary logs, and a
recomputation over the four audit files with `.scratch/pota-repro/t330/lead_verify_flips.py` and
`lead_verify_joins.py`.

**Control, read at the source.** `..._t330-potE-flips-i/logs/binary_S60.log` ends
`Sherd accuracy : 31 / 31 (100%)` / `Edge accuracy : 62 / 62 (100%)`, 1815 candidates, 796 pruned;
`binary_S9277.log` ends `29 / 31 (93.5484%)` / `53 / 62 (85.4839%)`, 1815 candidates, 817 pruned.
Jobs 34749879 / 34749880, both COMPLETED 0:0, 00:20:40 / 00:18:01. One binary, argv[2] the only
difference, regression reproduced: **verified**.

**The flip table, recomputed.** Pair level (a pair flips when its number of surviving candidates
goes from none to some, or the reverse): MOR1 **13 pairs, none of them GT**; K3R1 **10 pairs, none
GT** - the worker said 13 in both lanes, so K3R1 is 10 by this definition (wording correction only,
the answer is unchanged). Candidate level, 138 candidates across 107 pairs change verdict, so the
candidate pool is unchanged at the enumeration but not at the pruning: the filter is sequential, so
a shifted threshold reorders the basket and moves candidates whose own gate did not change (34 MOR1
candidates go survivor -> `axis_angle>0.436`). That is the mechanism the worker describes, seen in
its own data.

**Two numbers corrected.**

1. The audit files are in run dir `..._t330-potE-flips-h/out/`, 701,966 / 708,657 / 593,178 /
   603,319 bytes - not under the paths the ticket listed. Corrected above, so the evidence is
   findable.
2. GT-correct candidate surviving in **both** regimes among the 16 failed MOR1 joins: **14 of 16**,
   not 12. The two exceptions are 14-26 (every candidate pruned in both regimes) and 26-30 (**no
   candidate is ever enumerated** - invisible to every threshold, a fact worth its own line in any
   later ticket about it).

**P25-P26, the link that carries the damage.** Both candidates survive in both regimes: idx 1794
(score 0.375, gap 0.16 mm, **t_err 55.1 mm**) and idx 1795 (0.192, gap 0.12 mm, **t_err 86.4 mm**).
Neither is GT, and both were available to the pristine search. Confirmed.

**Ruling on the pre-registered rule.** Clause (a) fails on its second half and (b) fails outright;
the substance of (c) is what the data shows - 13 pairs flip in MOR1, none GT, none on the path of a
lost join, and the wrong link that detaches {P25,P26,P31} was available in both regimes.
**The scaled gates did not admit the wrong join: the search made a different choice from an
essentially unchanged pool.** Therefore the rule "a millimetre must not be scaled by object size"
is NOT established by this route; what the patch demonstrably moves is the ranking and merge
outcome, through the rescaled inlier distance and outlier rejection inside Icp.

**Deviation accepted once.** The arms ran on `interactive` (8 cpus, <= 21 min) because sapphire
started nothing in two hours; the redirect of >8-cpu interactive requests to sapphire (probes
34749308 / 34749408 / 34749409) shows the boundary is policed. Tolerated here - a short, single-node
measurement, recorded - and not to become the pattern.

**Consequence for ticket 329.** The count floor is a different lever from the length gate and the
ten-pot bar of 329 decides it. This ticket is the caution: the search is sensitive to any shift in
scores or consensus, so 329 must run its ten-pot bar rather than reason that only one gate moved.


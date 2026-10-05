# 12: Assembly nondeterminism — same tree, 2/8 then 0/8

**Answers:** S1

**Blocked by:** nothing — reruns + code inspection; no new method needed
to start

**Status:** resolved 2026-10-05 — node-dependent FP confirmed; same-node
pair identical. Standing rule: pin nodes for every assembly A/B.

**Needs-eye:** none — accuracy distributions and root-cause code paths.

## The finding (2026-10-02, job 32107435)

Behaviorally-identical trees, different accuracy: job 31898200 (paper
values + stddev + T0 + Reg-150) placed pieces 2,3 (2/8); job 32107435
(same + print/comment/dead-code deletions, all verified no-behavior)
placed nothing (0/8, 15 states, best 249, profile 68/0). Nothing that
changed between the runs can change matching behavior — unless the
pipeline is nondeterministic run-to-run on identical inputs. If it is,
EVERY single-run A/B verdict in tickets 07/08 (E1–E3, V1, V2a/b,
control-150, paper-config-vs-baseline) is suspect: the consistent 0/8s
are robust-ish (failure repeated 8+ times), but the lone 2/8 and the
control's "iterations load-bearing" may be draws from a nondeterministic
distribution, not effects.

## CODE INSPECTION 2026-10-02 (default path — suspects narrowed)

- OMP `parallel for` (data_structure :790, reconstruction :518/:582/:645/:728):
  all index-partitioned writes; only shared op is `cor_counter++`
  (never read — benign race, UB-technically, behavior-neutral).
  `feature_matching` OMPs are commented out. NOT the source (so far).
- RANSAC nondeterminism offline-only; sorts deterministic; no RNG/time
  seeds live; trans/s zeroed every iter; Tables zeroed; scores init 0.
- MALLOC_PERTURB_ experiment queued (see below) — distinguishes heap-garbage
  reads from threading without any code change.
- RANSAC nondeterminism (`random_device` + shared mt19937 + unordered_set
  ordering, axis_estimation.cpp:24/138-186): OFFLINE TOOL ONLY
  (`ComputePotSACAxis`, sole caller the edgeline-extraction tool).
  Excluded for production runs. Noted for the record.
- Sorts: `std::list::sort` (stable) + `std::sort` on vectors (deterministic
  given fixed input) — need unordered INPUT to matter; none found in the
  live path (`intersection_detector` unordered_map include is unused in
  its .cpp; puzzlefusion unordered_sets are dead-by-default).
- `time()` (ranking_system :525): filenames only. `rand()`: puzzlefusion
  only (dead by default). No RNG/time seeds in live behavior.
- REMAINING suspects: Ceres/Eigen thread FP summation order (weak alone,
  needs near-ties to flip argmins — beam scores may provide them);
  uninitialized reads (not yet swept — Eigen matrices sized-then-
  partially-filled + full reads are the pattern to hunt).
- Empirical test queued: guard run (32109955) + identical repeat, compare.
  If identical → pipeline deterministic → 2/8-vs-0/8 needs tree-level
  re-explanation (re-audit the two run trees hunk by hunk). If different
  → nondeterminism confirmed, hunt continues above.

1. QUANTIFY (no code change): rerun the SAME binary + SAME dataset 3×
   (the guard run 32109955 counts as run 1 of the new binary; two more
   identical submits). Report the accuracy distribution. If 0/8 ×3 with
   varying states/scores → nondeterministic search on deterministic
   matching (matcher proven byte-identical on rerun). If 2/8 reappears →
   bimodal outcome, worse (flaky success).
2. LOCATE (code inspection): candidate nondeterminism sources — OMP
   `parallel for` with unordered reductions in/around ICP and beam
   (`NUMBER_OF_THREAD`, `parallel for` pragmas — grep them all);
   uninitialized memory (vectors sized but not set before read);
   iteration over `std::unordered_*` containers; time/PID-seeded RNGs;
   `rand()`/`srand` anywhere. List every site with file:line, then
   instrument-or-fix ONE (fixed seed? single-thread A/B? initialize?).
3. FIX CANDIDATE (only after 1–2): determinism is a prerequisite for
   every A/B in 06/07/08 — a nondeterministic matcher cannot be tuned
   (each measurement is a draw). The fix that makes reruns byte-identical
   outranks all tuning. Until then, every future A/B needs ≥2 runs per
   arm with the distribution reported, not single numbers.

## RESOLVED 2026-10-05 (node-pinned pair identical — node-dependent FP)

Node-pinned pair on bm179 (jobs 32155220 + 32273451): IDENTICAL on every
number — 0/8, 0/15, best 235.000 (1+6+1), 13 states, profile 40/1,
matching/pruning md5s identical. Against cross-node pairs that always
differ in fringe (and sometimes argmax): the source is NODE-DEPENDENT
floating point (CPU-arch vector paths in Eigen/Ceres/VTK code compiled
once, run on heterogeneous Sapphire nodes), not threads (all off),
not heap garbage (perturb didn't fix), not code races (none found live).
Standing rule (operational fix — no code change can fix CPU-arch FP):
pin `--nodelist` for every assembly A/B; cross-node numbers are draws
across microarchitectures, same-node numbers are comparable. All past
cross-node A/B verdicts keep their failure direction (0/8 repeats) but
lose their positive claims to draws-not-effects until same-node
repeated — this now applies as stated, no per-ticket re-verdict needed
beyond what ticket 12 already recorded.
(Rule-6 note, AGENTS.md 2026-10-05: all compute already runs on holders;
the ticket's laptop-side analyses predate the rule.)

Identical binary + identical data: guard run 0/8 + 0/15 (19 states, best
335, 73/0, 550 diverged) vs repeat 2/8 + 1/15 (best 283, State 0 = 2+5+1,
86/0, 572 diverged). Accuracy flips run to run; profile kills nothing in
either. The difference is which merges form and which poses land — the
search and solver, not the gates.

## THREADING + PERTURB PAIRS 2026-10-03: best stable-ish, fringe wanders (verified table)

Same binary + same data throughout (matching md5 51b57b85 and pruning
md5 43920151 IDENTICAL in all six runs — divergence enters strictly
after pairwise pruning):

| pair | run A best | run B best | fringe |
|---|---|---|---|
| Ceres-1 (threaded Eigen) | 246.000, 1+6+1 | 246.000, 1+6+1 | states 18/19, profile 53/57, div 571/583 |
| Eigen-off + Ceres-1 | 246.000, 1+6+1 | 261.000, 7+1 | best MOVED (not stabilized) |
| Perturb (fixed heap) | 235.000, 1+6+1 | 235.000, 1+6+1 | profile 40/41, div 545/544 |

Threading FP order is NOT the (sole) source (fully single-threaded pairs
still differ); fixed heap fill doesn't fix the fringe either. Best-state
stability varies pair to pair with no threading correlation. Remaining:
node-dependent FP (CPU-arch vector paths — every pair so far ran on
DIFFERENT nodes: bm179/bm162, bm209/bm186, bm207/bm210...) vs
stack-garbage reads. NODE-PINNED pair running (job 32155220 on bm179;
twin follows): same node twice decides it. If identical → node/CPU-arch
FP confirmed (pin nodes for A/Bs + record arch sensitivity; cross-node
bit-identity may be unachievable, same-node determinism is the
operational bar). If different → stack garbage or true races;
valgrind-grade hunt next.
CONSEQUENCE (bulk re-verdict): every single-run A/B in 07/08 stands as
single-run evidence with stated caveats — consistent 0/8s remain failure
evidence (repeated 8+ times); the 2/8 positive, the control's "iterations
load-bearing", and V1/V2a/b deltas are DRAWS, not effects, until
repeated. Determinism fix outranks all tuning.

## Out of scope

- Re-litigating E1–V2b verdicts individually: they stand as
  single-run evidence with the stated caveats until 1–2 redirect them.
  If determinism is confirmed broken, a BULK re-verdict follows here
  (which conclusions survive repeat runs), not in each ticket.
- The guard (ticket 11): separate variable, already running (job
  32109955). If guard + determinism interact (guard stabilizes by
  killing divergent branches?), that is a finding FOR this ticket.

## Acceptance criteria

- [ ] Distribution over ≥3 identical reruns reported (accuracy + states +
      best score each)
- [ ] Nondeterminism source located (or determinism PROVEN by 3×
      identical — equally an answer, and the cheaper one to hope for)
- [ ] If broken: fix + 3× identical reruns; bulk re-verdict on E1–V2b
- [ ] MALLOC_PERTURB_ pair (no code change): two runs with
      MALLOC_PERTURB_=169 (heap garbage fixed) — identical ⇒ uninit heap
      reads confirmed; still different ⇒ threading (Eigen test decides
      between them; perturb-test is independent of it)
- [ ] Until fixed: standing rule that every assembly A/B reports
      distributions (this ticket states it; tickets cite it)

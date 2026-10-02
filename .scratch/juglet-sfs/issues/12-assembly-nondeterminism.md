# 12: Assembly nondeterminism — same tree, 2/8 then 0/8

**Answers:** S1

**Blocked by:** nothing — reruns + code inspection; no new method needed
to start

**Status:** ready-for-agent

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

## What to establish (in this order)

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
- [ ] Until fixed: standing rule that every assembly A/B reports
      distributions (this ticket states it; tickets cite it)

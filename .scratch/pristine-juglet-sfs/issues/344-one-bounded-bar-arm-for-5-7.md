# Ticket 344 — one bounded bar arm: does admitting 5-7 buy accuracy

**Status:** needs-info
**Working in:** chat on Thinkbook16 (2026-11-14)
**Branch:** to be created at start by `scripts/ticket_dispatch.sh`; work only in the worktree it names
**Answers:** S1
**Depends on:** 341 (the numbers behind the refusals), 340 (which candidate is applied), 337 (widening the branch window did not help)
**Waits for:** ticket 348 (the measured contact set)
## Why this stopped, and why it is now waiting (2026-11-14)

This ticket was stopped on the reading that **5-7 is not a contact**, taken from the answer key's
edge list. **That reading is wrong, and the ticket resumes as a waiting probe.** Ticket 347
measured the mating at the true pose: 5-7 is back-to-back - 459 vertices within 0.1 mm, median
normal dot -0.910, 75.4% opposed, over 29.3 mm - the same signature as the listed 6-7 and 1-8;
4-7 is not (+0.448, 4.7%). The key's list fails on nine of its own 18 entries (those stand 3-19 mm
apart at the true pose), so its silence about 5-7 is not evidence about 5-7.

The question the ticket was written for is therefore still open, but not as it was posed. **5-7 is
accepted at step 0** - it is in the winning assembly in both repeats (ticket 342, edge #160) - and
refused at steps 1-3, where the group profile has grown to 8.86 mm against the 7.0 mm bar. So the
question is not "should the gate admit 5-7" (it does) but whether refusing that edge in the later
states costs accuracy. The bounded bar arm (`SFS_PC_BAR` 7.0 -> 9.0, this pair only) is the probe
that answers it, and its outcome must now be read against the **measured** contact set, not the
key's list.

It waits for ticket 348 (the measured set and the re-scored denominators). If 348 shows the join
count was never the issue, this arm runs as written; if 348 shows the shape number is measuring
the wrong thing, the ticket is rewritten rather than run.

Recorded in S1, section '2026-11-14 - The join the gate refused is a join; the reference is not a
contact list'.

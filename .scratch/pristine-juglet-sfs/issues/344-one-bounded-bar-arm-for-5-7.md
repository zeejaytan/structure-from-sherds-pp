# Ticket 344 — one bounded bar arm: does admitting 5-7 buy accuracy

**Status:** wontfix
**Working in:** chat on Thinkbook16 (2026-11-14)
**Branch:** to be created at start by `scripts/ticket_dispatch.sh`; work only in the worktree it names
**Answers:** S1
**Depends on:** 341 (the numbers behind the refusals), 340 (which candidate is applied), 337 (widening the branch window did not help)
**Waits for:** nothing - stopped 2026-11-14, see 'Why this stopped' below
## Why this stopped (2026-11-14)

Ticket 345 gave this ticket its premise back - the 5-7 join is already 8.9 mm from its own
profile when it is made - but the premise itself does not survive the reference check.
**5-7 is not a contact.** The answer key's contact graph
(`SfSpp_preprocessing/artifacts/67/juglet_gt/Juglet_simple_graph.txt`, 18 edges) does not list
the pair, and the ten pairs ticket 326 measured as touching at the true pose do not include it.

So candidate 160 (4.71 deg / 3.69 mm, `score_` .322249, inlier 72) is a *false positive*: a
plausible relative pose of two sherds that never met. The gate's eight refusals at 8.856-8.860
mm were right, and `SFS_PC_BAR` 7.0 -> 9.0 for this pair would not be a tuning experiment; it
would be teaching the gate to admit a join the pot does not have. A bar that admits 8.86 mm
would also admit the 8.9-11.3 mm band on 4-7, which is a false positive too.

Nothing is lost by stopping it: the question it was written for (is the profile bar too tight on
a join inside the 5 deg / 5 mm bar) is now a question about *touching* pairs, and the one touching
pair with a bar refusal is 6-9, whose applied edge has no candidate row at all (tickets 341,
346) - a different ticket.

Recorded in S1, section '2026-11-14 - The join the gate refused is not a join'.

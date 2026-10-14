# 03: Score, witness render, verdict into S3

**What to build:** the run's numbers scored against the honest denominator, the
reassembly looked at (not just counted), and S3 updated — so the loop closes in
`intent/`, not in a log.

**Answers:** S3

**Blocked by:** 02 (needs the run output).

**Needs-eye:** `visual-qa/preview/manifest-juglet-pristine.json` (stage the pair with the visual-qa-helper; mesh/point-cloud pair, never a bare picture)

**Status:** ready-for-agent

- [ ] SA/EA scored vs honest 0/10 denominator (and nominal /18 noted); V-B tolerances τR=20° τT=50mm
- [ ] Render staged at a view resolving the seam scale; witnessed look + conservator note recorded below (numbers alone do not close this ticket)
- [ ] S3 `Where it stands` + Done-when boxes updated with date, job IDs, scores, and the verdict (untouched agrees / diverges)
- [ ] Board rebuilt (`python scripts/build_board.py`) in the same commit

# 02: Pristine Juglet assembly run

**What to build:** a Spartan run of the untouched binary (upstream @9195c01 + Rank 2
shim, container recipe as-is) over the Juglet bundle via the ticket-01 block, at the
paper's k=5/b=3 — producing a join count the thesis can cite either way.

**Answers:** S3

**Blocked by:** 01 (data block + bind must land first).

**Status:** ready-for-agent

- [ ] Gate probe `gate_probe_b0.py juglet` recorded first (pass count at GT)
- [ ] Assembly submitted (shim build job + run job IDs recorded below with sacct State/ExitCode via `scripts/slurm_poll.sh`)
- [ ] Run config recorded: upstream SHA, k/b, bind flag, data block name
- [ ] Raw output (SA/EA, seated pairs, scores) pasted under Comments; no verdict here (verdict lives in S3 via ticket 03)

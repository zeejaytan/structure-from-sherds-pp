| job | partition | final sacct State | Elapsed | what it was / what it did |
|---|---|---|---|---|
| 34730091 | sapphire | CANCELLED | 00:00:00 | audit v1 (no `gap_min` column) — cancelled before it started |
| 34730267 | sapphire | CANCELLED | 00:00:00 | audit main variant — 2 h PENDING, never started |
| 34732537 | sapphire | CANCELLED | 00:00:00 | duplicate of 34730267 — never started |
| 34736268 | sapphire | CANCELLED | 00:00:00 | small variant with a prebuilt tree — never started |
| 34740219 | sapphire | CANCELLED | 00:00:00 | 1-cpu variant — never started |
| 34740760 | cascade | COMPLETED | 00:00:02 | cascade attempt; sbatch printed an admin advisory steering jobs to sapphire, superseded |
| 34740841–44 | sapphire | CANCELLED | 00:00:00 | four one-label jobs — never started |
| 34742068 | sapphire | CANCELLED | 00:00:00 | 1-cpu hello probe — never started |
| 34746227 | interactive | COMPLETED | 00:00:03 | audit attempt; script path bug → exit 127, no output |
| 34746349 | interactive | COMPLETED | 00:00:58 | **the audit** — wrote all four `audit_*.txt` |
| 34748598 | sapphire | CANCELLED | 00:00:00 | assembly submit; silently redirected off interactive (see the deviation note) |
| 34748983 | sapphire | CANCELLED | 00:00:00 | assembly submit with `-p interactive` on the command line; redirected the same way |
| 34749308 | interactive | COMPLETED | 00:00:00 | `--wrap` probe at 8 cpus — stayed on interactive |
| 34749408 | sapphire | CANCELLED | 00:00:00 | `--wrap` probe at 16 cpus — **redirected to sapphire**, which is the proof of the 8-cpu rule |
| 34749409 | interactive | COMPLETED | 00:00:00 | `--wrap` probe at 8 cpus — stayed on interactive |
| 34749577 | interactive | COMPLETED | 00:00:11 | S60 arm; link failed (relative object paths — see the deviation note) |
| 34749578 | interactive | COMPLETED | 00:00:12 | S92.77 arm; same link failure |
| 34749879 | interactive | COMPLETED | 00:20:40 | **S60 whole assembly** → 31/31 sherds, 62/62 edges |
| 34749880 | interactive | COMPLETED | 00:18:01 | **S92.77 whole assembly** → 29/31 sherds, 53/62 edges |

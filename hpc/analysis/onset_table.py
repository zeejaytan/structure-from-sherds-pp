"""Divergence onset table (ticket 11 forensics).

Joins ICPCOR iter=0 (pts, meandist) with the following REGOUT t_norm per
pair, sequentially. Prints the joint distribution: do insane outcomes
share a pre-condition (few pts / large meandist)?
Usage: onset_table.py <log> [...]  (run on holder)
"""
import math
import re
import sys
from collections import defaultdict


def main():
    # (pts_bin, dist_bin) -> [insane, sane]
    tab = defaultdict(lambda: [0, 0])
    pending = {}  # pair -> (pts, meandist) from latest iter-0
    for path in sys.argv[1:]:
        with open(path, errors="replace") as fh:
            for ln in fh:
                m = re.search(
                    r"ICPCOR .* pair (\d+)-(\d+) iter=0 objs=\d+ "
                    r"pts=(\d+) meandist=([0-9.]+)", ln)
                if m:
                    pending[(m.group(1), m.group(2))] = (
                        int(m.group(3)), float(m.group(4)))
                    continue
                m = re.search(
                    r"REGOUT .* [Pp]ieces (\d+)-(\d+) t_norm=([0-9.e+-]+)",
                    ln)
                if m:
                    key = (m.group(1), m.group(2))
                    if key not in pending:
                        continue
                    pts, md = pending.pop(key)
                    try:
                        v = float(m.group(3))
                    except ValueError:
                        continue
                    pb = "pts<10" if pts < 10 else (
                        "10<=pts<50" if pts < 50 else "pts>=50")
                    db = ("d<5" if md < 5 else "5<=d<20" if md < 20
                          else "20<=d<100" if md < 100 else "d>=100")
                    tab[(pb, db)][0 if v > 2000 else 1] += 1
    print(f"{'pts':12} {'meandist':12} {'insane':>8} {'sane':>8} "
          f"{'insane%':>8}")
    for (pb, db) in sorted(tab):
        i, s = tab[(pb, db)]
        t = i + s
        print(f"{pb:12} {db:12} {i:8d} {s:8d} "
              f"{(100.0 * i / t if t else 0):7.1f}%")
    # unmatched iter-0s (attempts that never reached REGOUT)
    print(f"unmatched pending: {len(pending)}")


if __name__ == "__main__":
    main()

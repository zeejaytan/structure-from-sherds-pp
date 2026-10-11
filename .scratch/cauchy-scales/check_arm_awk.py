#!/usr/bin/env python3
"""Local validation of the awk blocks used by run_e2_graduated.sbatch.

The arm's primary read is two gawk one-liners that concatenate a column into a
space-separated string, split() it, asort() it and index percentiles. Concatenating
millions of values into one string is fragile, so this script verifies the same
logic locally against a synthetic dump before the job spends 45 minutes of compute
to discover a syntax error.

Usage: python check_arm_awk.py
"""
import os
import subprocess
import sys

HEADER = ("site\titer\tblock_index\tclass\tdim\ta\tfnorm\ts_a2\trho_p"
          "\traw_unit\traw_norm\tcor_pair\tcor_point")


def synth_rows():
    rows = [HEADER]
    # Icp iter 0: 5 dist rows at a=260 with fnorm 100..104 -> s/a2 ~0.148
    for i in range(5):
        fn = 100.0 + i
        a = 260.0
        s = fn * fn
        sa2 = s / (a * a)
        rows.append("\t".join(["Icp", "0", str(i), "dist_line4", "3", str(a),
                               "%.6f" % fn, "%.8f" % sa2,
                               "%.8f" % (1.0 / (1.0 + sa2)), "%.6f" % fn,
                               "%.6f" % fn, "0", str(i)]))
    # Icp iter 0: 3 norm rows at a=1.8 (must NOT appear in the dist read)
    for i in range(3):
        fn = 0.2 + i * 0.05
        a = 1.8
        s = fn * fn
        sa2 = s / (a * a)
        rows.append("\t".join(["Icp", "0", str(i), "norm", "3", str(a),
                               "%.6f" % fn, "%.8f" % sa2,
                               "%.8f" % (1.0 / (1.0 + sa2)), "%.6f" % fn,
                               "%.6f" % fn, "0", str(i)]))
    # Registration5 iter 1 (filtered out by $2==0)
    rows.append("\t".join(["Registration5", "1", "0", "dist_line4", "3", "181.0",
                           "300.0", "2.75", "0.2667", "300.0", "300.0", "0", "0"]))
    return "\n".join(rows) + "\n"


AWK = r"""
  NR>1 && $2==0 && ($4=="dist"||$4=="dist_line2"||$4=="dist_line4"||$4=="dist_other"){
    n[$1]++; v[$1]=v[$1]" "$8
  }
  END{
    for (s in n) { split(v[s], a, " "); asort(a); delete a[1];
      k=int((n[s]-1)*0.5)+1; k90=int((n[s]-1)*0.9)+1;
      printf "    %-16s n=%-7d p50=%-11.4g p90=%-11.4g max=%-11.4g\n", s, n[s], a[k], a[k90], a[n[s]] }
  }
"""


def main():
    tmp = os.path.join(os.environ.get("TEMP", "/tmp"), "dump_e0_0000.tsv")
    with open(tmp, "w", encoding="utf-8") as fh:
        fh.write(synth_rows())
    ok = True
    for col, label in ((8, "s/a^2"), (9, "rho'")):
        awk = AWK.replace('" "$8', '" "$%d' % col)
        p = subprocess.run(["awk", "-F", "\t", awk, tmp],
                           capture_output=True, text=True)
        print("=== %s ===" % label)
        print(p.stdout.rstrip() or "(no output)")
        if p.stderr.strip():
            print("STDERR:", p.stderr.strip())
            ok = False
    if ok:
        os.remove(tmp)
        print("awk blocks parsed and ran clean")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())

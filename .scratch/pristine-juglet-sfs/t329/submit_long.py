
import os, re, shutil, subprocess
base = "/data/gpfs/projects/punim2657/SfSpp/runs"
src_tpl = "20261111-0230_20b52bf_t329-pot%s-8c"
dst_tpl = "20261111-0230_20b52bf_t329-pot%s-8cL"
pots = list("ABCDEFGHIJ")
TL = {"A":"01:00:00","B":"01:00:00","C":"00:30:00","D":"06:00:00","E":"06:00:00",
      "F":"00:30:00","G":"00:30:00","H":"01:00:00","I":"03:30:00","J":"01:00:00"}
for p in pots:
    s = os.path.join(base, src_tpl % p); d = os.path.join(base, dst_tpl % p)
    if os.path.exists(d): shutil.rmtree(d)
    shutil.copytree(s, d, symlinks=True)
    f = os.path.join(d, "submit.sbatch")
    t = open(f).read()
    t = t.replace("--partition=sapphire", "--partition=long")
    t = t.replace("#SBATCH --job-name=t329-pot%s" % p, "#SBATCH --job-name=t329-pot%s-8cL" % p)
    t = re.sub(r"#SBATCH --time=\S+", "#SBATCH --time=" + TL[p], t)
    t = t.replace("# Ticket 329 bar 1 step 2:",
                  "# Ticket 329 bar 1 step 2 (long-partition duplicate: sapphire fairshare 0.137, est >24 h):")
    open(f, "w").write(t)
    r = subprocess.run(["sbatch", "--partition=long", "submit.sbatch"], cwd=d, capture_output=True, text=True)
    print(p, r.returncode, r.stdout.strip(), r.stderr.strip()[:200])

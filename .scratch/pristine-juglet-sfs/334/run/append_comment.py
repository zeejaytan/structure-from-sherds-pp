import io
T = r"C:/PR/worktrees/structure-from-sherds-pp/334/.scratch/pristine-juglet-sfs/issues/334-upside-down-proposed-or-outscored.md"
C = r"C:/PR/worktrees/structure-from-sherds-pp/334/.scratch/pristine-juglet-sfs/334/run/comment334.md"
t = io.open(T, encoding="utf-8").read()
c = io.open(C, encoding="utf-8").read()
assert len(t.splitlines()) == 132, len(t.splitlines())
assert "## Comments (worker, 2026-11-12)" not in t, "already appended"
t = t.rstrip("\n") + "\n" + c
t = t.replace("**Status:** in-progress", "**Status:** results-in", 1)
io.open(T, "w", encoding="utf-8", newline="\n").write(t)
print("ok chars", len(t), "lines", len(t.splitlines()))
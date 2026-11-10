import io
p = r'C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pristine-juglet-sfs/issues/330-potE-which-gate-flipped.md'
s = io.open(p, encoding='utf-8').read()
old = '- Run dir: `/data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips` (main),\n  `-b`, `-c`, `-i` (assembly).'
new = ('- Run dir: `/data/gpfs/projects/punim2657/SfSpp/runs/20261111-0248_be65afc_t330-potE-flips` (main),\n'
       '  `-b`, `-c`, `-i` (assembly), `-h` (**the audit**: the four `audit_*.txt` are in ITS `out/`, not in the\n'
       "  main dir's - the main dir's `out/` and `logs/` are empty).")
assert old in s, 'run-dir line not found'
s = s.replace(old, new)
if '## Lead verification' not in s:
    s = s.rstrip() + '\n' + io.open(r'C:/PR/worktrees/structure-from-sherds-pp/330/.scratch/pota-repro/t330/lead_block.md', encoding='utf-8').read()
io.open(p, 'w', encoding='utf-8', newline='\n').write(s)
print('ok', len(s))
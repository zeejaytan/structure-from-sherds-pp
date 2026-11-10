t=r'C:\PR\worktrees\structure-from-sherds-pp\330\.scratch\pristine-juglet-sfs\issues\330-potE-which-gate-flipped.md'
d=r'C:\PR\worktrees\structure-from-sherds-pp\330\.scratch\pota-repro\t330\draft_comment.md'
s=open(t,encoding='utf-8').read()
draft=open(d,encoding='utf-8').read().rstrip()+chr(10)
assert '**Status:** in-progress' in s
assert '## Comments (worker, 2026' not in s
s=s.replace('**Status:** in-progress','**Status:** results-in',1)
s=s.rstrip()+chr(10)+chr(10)+draft
open(t,'w',encoding='utf-8').write(s)
print('ticket chars',len(s))
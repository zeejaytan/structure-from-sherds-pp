p=r'C:\PR\worktrees\structure-from-sherds-pp\330\.scratch\pota-repro\t330\draft_comment.md'
q=r'C:\PR\worktrees\structure-from-sherds-pp\330\.scratch\pota-repro\t330\ledger.md'
s=open(p,encoding='utf-8').read()
led=open(q,encoding='utf-8').read()
NL=chr(10)
old='- Every job id this worker submitted, with its final sacct state: see the table appended below.'
assert old in s
new='- Every job id this worker submitted, with its final sacct state (`sacct -X`, swept after the last job ended):' + NL + NL + led
s=s.replace(old,new,1)
a='rescaled inlier distance and outlier rejection *inside* Icp, a different merge order.'
assert a in s
b=a + ' End to end, the' + NL + 'same binary on the same tree scores **62/62 at s = 1.0 and 53/62 at s = 1.546** (jobs 34749879,' + NL + '34749880) - the regression reproduced with nothing but argv[2] changed.'
s=s.replace(a,b,1)
open(p,'w',encoding='utf-8').write(s)
print('chars',len(s))
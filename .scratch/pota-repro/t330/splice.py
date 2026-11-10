p=r'C:\PR\worktrees\structure-from-sherds-pp\330\.scratch\pota-repro\t330\draft_comment.md'
q=r'C:\PR\worktrees\structure-from-sherds-pp\330\.scratch\pota-repro\t330\insert.md'
s=open(p,encoding='utf-8').read()
ins=open(q,encoding='utf-8').read()
mark='- **12 of 16** failed joins'
i=s.index(mark)
s2=s[:i]+ins+s[i:]
open(p,'w',encoding='utf-8').write(s2)
print('chars',len(s2))
import re
print([m for m in re.findall(r'^#{2,3} .*',s2,flags=re.M)])
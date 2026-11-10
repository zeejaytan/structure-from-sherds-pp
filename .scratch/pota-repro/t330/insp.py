p=r'C:\PR\worktrees\structure-from-sherds-pp\330\.scratch\pota-repro\t330\_b7_full.txt'
s=open(p,encoding='utf-8').read()
key='**Every length literal the patch scales'
j=s.find(key)
print('key at',j)
m='### Bar 2 \u2014 the flip table'
k=s.find(m)
print('bar2 at',k)
a=s.find('- **12 of 16**')
print('bar3bullets at',a)
if k>0: print(repr(s[k-120:k+60]))
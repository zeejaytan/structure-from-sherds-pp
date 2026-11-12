import json,os
R='.'
NINE=[(1,2),(1,4),(1,8),(1,9),(2,5),(2,9),(3,5),(6,7),(7,9)]
print('arm          present pass  upside(>90deg)  min-angle(max)')
for r in (1,2):
  for a in ['gt18','gt09','gt09w12','ds18','ds09','ds09w12']:
    lab='%s_r%d'%(a,r)
    d=json.load(open(os.path.join(R,'score',lab+'.json')))
    pe={(e['i'],e['j']):e for e in d['tables']['per_edge']}
    pres=[pe[k] for k in NINE if k in pe]
    p=[e for e in pres if e['pass']]
    up=[e for e in pres if e['deg']>90]
    angs=[e['deg'] for e in pres]
    print('%-12s %d/9   %d/9   %d             %s'%(lab,len(pres),len(p),len(up), ('%.1f..%.1f'%(min(angs),max(angs))) if angs else '-'))
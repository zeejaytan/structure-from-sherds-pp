p=r'C:\PR\worktrees\structure-from-sherds-pp\330\.scratch\pota-repro\t330\draft_comment.md'
s=open(p,encoding='utf-8').read()
def blk(text,mark):
    i=text.index(mark)
    j=text.find(chr(10)+'- ', i+3)
    return text[i:j if j>0 else len(text)]
new_k3 = chr(10).join([
 '- **The end-to-end control is exact where it matters.** The whole assembly at S = 60 prints',
 '  31/31 sherds and 62/62 edges - the baseline\u2019s number, from the same binary that regresses at',
 '  S = 92.77. The two arms differ only in argv[2].',
 '- **K3R1 is not an exact control at the pruning stage.** The audit gives 836 at S = 60 where the',
 '  t323 K3R1 log says 839 - 3 candidates short, the size of the fringe described next. MOR1, the',
 '  lane bar 3 asks about, is exact (792 = the t323 log). K3R1 appears below only as a secondary lane.',
 '- **The two assembly arms\u2019 per-piece poses were not retained.** The batch script\u2019s final copy step',
 '  pointed at a container path that was never bind-mounted, and the next line removed',
 '  /Dataset/SfS_pp/Result, so only the logs survive. The printed accuracy is the assembler\u2019s own',
 '  CountResult output - the same code path that writes 1. Acc.txt - so the control number is',
 '  authoritative, but no per-piece claim is made from these two arms.'])
new_nd = chr(10).join([
 '- **The search has a small non-deterministic fringe, and it moves the join count.** Four',
 '  measurements of MOR1 at s = 1.546 (identical inputs, identical 1815 candidates) prune **814, 816,',
 '  816, 817** and score **46/62, 53/62, -, 53/62**: t326\u2019s own MOR1 and MOR2 arms (814 -> 46, 816 -> 53),',
 '  my audit (816, pruning only) and my assembly arm (817 -> 53). At s = 1.0 the same tree prunes 792',
 '  (t323 log and my audit) and 796 (my assembly), and scores 62/62 either way.',
 '  class/reconstruction.cpp has no srand/rand; the likely cause is thread-order floating point inside',
 '  Icp, unverified. So **the size of the regression is not reproducible run to run** (46 vs 53 of 62)',
 '  even though its existence is: 62/62 at s = 1.0 and 53/62 at s = 1.546, same binary, same tree,',
 '  same candidate enumeration. Every candidate-level claim below names a candidate index, and those',
 '  indices and their (y, x, gt) triples are identical in both regimes.'])
b1=blk(s,'- **K3R1 is not an exact control.**')
s=s.replace(b1,new_k3,1)
b2=blk(s,'- **The pruning stage has a small non-deterministic fringe.**')
s=s.replace(b2,new_nd,1)
s=s.replace('| full assembly S = 60 (job 34749879) | {{S60}} |','| **whole assembly, S = 60** (job 34749879) | **31/31 sherds, 62/62 edges**, 1815 candidates / 796 pruned, COMPLETED exit 0:0 in 20:40 |')
s=s.replace('| full assembly S = 92.77 (job 34749880) | {{S9277}} |','| **whole assembly, S = 92.77** (job 34749880) | **29/31 sherds, 53/62 edges**, 1815 candidates / 817 pruned, COMPLETED exit 0:0 in 18:01 |')
s=s.replace('{{S9277SCORE}}','53/62')
assert '{{' not in s
open(p,'w',encoding='utf-8').write(s)
print('ok chars=%d'%len(s))
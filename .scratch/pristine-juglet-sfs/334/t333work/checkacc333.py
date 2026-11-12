#!/usr/bin/env python3
"""checkacc333.py ACCFILE - print SHERD=k/t EDGE=k/t and exit 0 only when both are complete."""
import re, sys
txt = open(sys.argv[1], errors='ignore').read()
nums = re.findall(r'Sherd accuracy\s*:\s*(\d+)\s*/\s*(\d+)', txt)
if len(nums) < 2:
    print('PARSE_FAIL')
    sys.exit(2)
s, e = nums[0], nums[1]
print('SHERD=%s/%s EDGE=%s/%s' % (s[0], s[1], e[0], e[1]))
sys.exit(0 if (s[0] == s[1] and e[0] == e[1]) else 1)

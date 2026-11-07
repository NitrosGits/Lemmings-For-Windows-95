#!/usr/bin/env python3
"""Try a different body for one function and compare it with the original:

  variant.py MODULE SOURCE 'signature line' BODYFILE [match.py options]

The function whose definition starts with the signature line gets BODYFILE's
text (everything between its braces); the result is compiled in build/scratch.
"""
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
mod, src, sig, bodyfile = sys.argv[1:5]
s = open(src).read()
start = s.index(sig + '\n{')
end = s.index('\n}\n', start)
s = s[:start] + sig + '\n{\n' + open(bodyfile).read().rstrip('\n') + s[end:]
out = os.path.join(ROOT, 'build', 'scratch', os.path.basename(src))
os.makedirs(os.path.dirname(out), exist_ok=True)
open(out, 'w').write(s)
name = sig.split('(')[0].split()[-1].lstrip('*')
r = subprocess.run([os.path.join(ROOT, 'tools', 'msvc', 'try.sh'), mod, out, '-f', name] + sys.argv[5:],
                   capture_output=True, text=True)
print(r.stdout.rstrip())

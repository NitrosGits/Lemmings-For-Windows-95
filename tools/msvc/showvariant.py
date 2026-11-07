#!/usr/bin/env python3
"""Print the side-by-side diff (ours | original) of ONE combination of a realname.py-style spec.

  showvariant.py MODULE SOURCE ADDR NAME SPEC.py COMBO [--all]
COMBO is realname.py's name for the combination without the leading c: with ONE slot any number
(24 = its 24th alternative); with several slots one digit per slot ('02') or comma-separated ('0,12'). --all prints
every row, not just the differing ones.
"""
import os, re, runpy, subprocess, sys, tempfile, shutil

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
mod, src, addr, name, spec_path, combo = sys.argv[1:7]
show_all = '--all' in sys.argv
spec = runpy.run_path(spec_path)
src = os.path.abspath(src)
text = open(src, encoding='latin1').read()
st0 = text.index('/* ' + addr); s = text.index('\n{\n', st0) + 3; e = text.index('\n}\n', s)
body = text[s:e]
ks = [int(combo)] if len(spec['SLOTS']) == 1 else [int(x) for x in (combo.split(',') if ',' in combo else combo)]
for (o, alts), k in zip(spec['SLOTS'], ks):
    body = body.replace(o, ([o] + alts)[k], 1)
d = os.path.join(ROOT, 'build/scratch/showvariant'); os.makedirs(d, exist_ok=True)
p = os.path.join(d, os.path.basename(src))
open(p, 'w', encoding='latin1').write(text[:s] + body + text[e:])
out = subprocess.run(['zsh', os.path.join(ROOT, 'tools/msvc/try.sh'), mod, p, '-v', '-f', name],
                     cwd=ROOT, capture_output=True, text=True).stdout.splitlines()
lo = int(addr, 16)
for l in out:
    m = re.match(r'\s*\*?\s*([0-9a-f]{6,8})\s', l)
    if l.startswith('0x') or (m and (show_all or l.lstrip().startswith('*'))):
        print(l[:150])

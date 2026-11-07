#!/usr/bin/env python3
"""Turn a module's export .def (ExportName = symbol @ord) into a dlltool import
.def that maps the C symbol back to the original export name (symbol == ExportName)."""
import re
import sys

src, dst = sys.argv[1], sys.argv[2]
out = []
for line in open(src):
    m = re.match(r'\s*(\S+)\s*=\s*(\S+)', line)
    if m and not line.strip().startswith(('LIBRARY', 'EXPORTS')):
        out.append(f'  {m.group(2)} == {m.group(1)}')
    else:
        out.append(line.rstrip('\n'))
open(dst, 'w').write('\n'.join(out) + '\n')

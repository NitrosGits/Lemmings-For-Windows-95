#!/usr/bin/env python3
"""Compile SOURCE with Visual C++ 2.2 and compare it with the original module:

  try.py MODULE SOURCE.c [match.py options, e.g. -v -f NAME]

Works on Windows (the compiler runs directly), macOS and Linux (under Wine);
see msvc.py. tools/msvc/try.sh is the same command for zsh. MSVCFLAGS
replaces the default /Ox. The object goes to build/msvc/try/<dir>/<name>.obj.
"""
import os
import re
import subprocess
import sys

import msvc

ROOT = msvc.ROOT
# CL.EXE echoes the source file's name (with its drive letter); drop that line
ECHO = re.compile(r'^[A-Z]:[^(]*\.(c|cpp)$', re.I)


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__.strip())
    mod, src, rest = sys.argv[1], os.path.realpath(sys.argv[2]), sys.argv[3:]
    out = os.path.join(ROOT, 'build', 'msvc', 'try', os.path.basename(os.path.dirname(src)),
                       os.path.splitext(os.path.basename(src))[0] + '.obj')
    os.makedirs(os.path.dirname(out), exist_ok=True)
    if os.path.exists(out):
        os.remove(out)
    flags = os.environ.get('MSVCFLAGS', '').split() or ['/Ox']
    rc, text = msvc.run(['/nologo', '/c'] + flags + ['/Fo' + out, src], os.path.dirname(out))
    for line in text.splitlines():
        if not ECHO.match(line):
            print(line)
    if not os.path.exists(out):
        print('compile failed')
        sys.exit(1)
    sys.stdout.flush()
    sys.exit(subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'match.py'), mod, src, out] + rest).returncode)


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Run one of Visual C++ 2.2's tools (CL.EXE, LINK.EXE, RC.EXE) on any platform:

  msvc.py ARGS...            TOOL=LINK.EXE msvc.py ARGS...   (default CL.EXE)

On Windows the tool runs directly. Under MSYS2 or Cygwin Python it also runs
directly, with POSIX paths in the arguments converted by cygpath. On macOS and
Linux it runs under Wine, in its own prefix, with Unix paths converted to the
Z: drive. The compiler is not part of this project: copy BIN, INCLUDE and LIB
from your own Visual C++ 2.2 into build/vc22/MSVC20 (or set VCDIR).

  VCDIR     the compiler folder (default build/vc22/MSVC20)
  VCPREFIX  the Wine prefix (default build/wine-vc; macOS/Linux only)
  WINE      the Wine command (default wine)

Other tools import run() instead of starting this script.
"""
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
VC = os.environ.get('VCDIR', os.path.join(ROOT, 'build', 'vc22', 'MSVC20'))

if os.name == 'nt':
    MODE = 'native'
elif sys.platform.startswith(('cygwin', 'msys')):
    MODE = 'cygwin'
else:
    MODE = 'wine'


def winpath(p):
    """A path as the Windows tool sees it."""
    p = os.path.abspath(p)
    if MODE == 'native':
        return p
    if MODE == 'cygwin':
        return subprocess.run(['cygpath', '-w', p], capture_output=True, text=True).stdout.strip()
    return 'Z:' + p.replace('/', '\\')  # Wine's default Z: drive is the Unix root


def convert(a):
    """Convert the paths in one argument, as tools/msvc/cl.sh always has."""
    if MODE == 'native':
        return a
    if a.startswith('/') and '/' in a[1:] and os.path.exists(os.path.dirname(a)):
        return winpath(a)                          # an absolute path
    if a[:3] in ('/Fo', '/Fa', '/Fe', '/Fp') and a[3:4] == '/':
        return a[:3] + winpath(a[3:])              # /Fo/abs/path
    if not a.startswith('/') and os.path.exists(a):
        return winpath(os.path.realpath(a))        # an existing relative path
    return a


def command(tool, args):
    exe = os.path.join(VC, 'BIN', tool)
    if MODE == 'wine':
        return [os.environ.get('WINE', 'wine'), exe] + [convert(a) for a in args]
    return [exe] + [convert(a) for a in args]


def environment():
    env = dict(os.environ)
    env['INCLUDE'] = winpath(os.path.join(VC, 'INCLUDE')) + ';' + winpath(os.path.join(ROOT, 'src', 'include'))
    env['LIB'] = winpath(os.path.join(VC, 'LIB'))
    if MODE == 'wine':
        env['WINEPREFIX'] = os.environ.get('VCPREFIX', os.path.join(ROOT, 'build', 'wine-vc'))
        env['WINEDEBUG'] = '-all'
    else:  # so CL.EXE finds its passes (C1, C2) and RC.EXE its RCDLL.DLL
        env['PATH'] = os.path.join(VC, 'BIN') + os.pathsep + env.get('PATH', '')
    if MODE == 'cygwin':
        # MSYS2 would rewrite /nologo, /c, /OUT:... into C:/msys64/... paths when
        # starting a Windows program; our paths are already converted by cygpath
        env['MSYS2_ARG_CONV_EXCL'] = '*'
        env['MSYS_NO_PATHCONV'] = '1'
    return env


def run(args, cwd, tool='CL.EXE'):
    """Run the tool in cwd; returns (exit code, its output with \\r removed). Wine's own messages are dropped."""
    if not os.path.exists(os.path.join(VC, 'BIN', tool)):
        sys.exit(f'{tool} not found in {os.path.join(VC, "BIN")}: copy Visual C++ 2.2 there or set VCDIR')
    r = subprocess.run(command(tool, args), cwd=cwd, env=environment(), stdout=subprocess.PIPE,
                       stderr=subprocess.DEVNULL if MODE == 'wine' else subprocess.STDOUT)
    return r.returncode, r.stdout.decode('latin-1').replace('\r', '')


if __name__ == '__main__':
    rc, out = run(sys.argv[1:], os.getcwd(), os.environ.get('TOOL', 'CL.EXE'))
    sys.stdout.write(out)
    sys.exit(rc)

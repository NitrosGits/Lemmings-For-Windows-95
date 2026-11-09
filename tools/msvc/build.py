#!/usr/bin/env python3
"""The matching build: every module compiled and linked with Visual C++ 2.2
the way the original was, then compared with the original byte for byte.

  build.py [MODULE ...]     default: every module with sources; MODULE is e.g. VSMEM
  build.py --funcs MODULE   also compare each annotated function (tools/match.py)

Output goes to build/msvc/<module>/. A game DLL whose sources aren't written
yet is stood in for (when linking the modules that import it) by a stub with
the same exports, so its import library matches the original's.
"""
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from pe import PE  # noqa: E402
import msvc  # noqa: E402  (runs the compiler: directly on Windows, under Wine elsewhere)

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, 'build', 'msvc')
SYSLIBS = os.path.join(OUT, 'lib')
CFLAGS = ['/nologo', '/c', '/Ox'] + os.environ.get('CFLAGS_EXTRA', '').split()  # CFLAGS_EXTRA: flag sweeps
DLLFLAGS = ['/DLL', '/BASE:0x10000000']

# 'out' is the file name the original was linked as (it is the name recorded in
# import tables), 'libs' the libraries after LIBC. A module's sources, in link
# order (the order their code appears in the original), are listed one per line
# in src/<module>/objs.txt.
MODULES = {
    'VSMEM':    dict(out='vsmem.DLL', libs=['KERNEL32', 'USER32']),
    'FILEHAND': dict(out='FILEHAND.DLL', libs=['KERNEL32', 'USER32']),
    'GAMEOVER': dict(out='GAMEOVER.DLL', libs=['KERNEL32', 'USER32']),
    'READDIB':  dict(out='readdib.DLL', libs=['FILEHAND', 'VSMEM', 'GDI32', 'KERNEL32', 'USER32']),
    'LOADLVL':  dict(out='LOADLVL.DLL', libs=['FILEHAND', 'READDIB', 'VSMEM', 'KERNEL32', 'USER32']),
    'RENDER':   dict(out='RENDER.DLL', libs=['READDIB', 'VSMEM', 'GDI32', 'WINMM', 'KERNEL32', 'USER32']),
    'PROCESS':  dict(out='PROCESS.DLL', libs=['FILEHAND', 'RENDER', 'VSMEM', 'WINMM', 'KERNEL32', 'USER32'],
                     flags=['/HEAP:4096,4096']),
    'LEMMINGS': dict(out='LEMMINGS.EXE', exe=True,
                     libs=['READDIB', 'RENDER', 'PROCESS', 'LOADLVL', 'VSMEM', 'FILEHAND',
                           'ADVAPI32', 'GDI32', 'KERNEL32', 'USER32', 'VERSION', 'WINMM']),
}
ORIGNAME = {k: (k + '.EXE' if v.get('exe') else k + '.DLL') for k, v in MODULES.items()}


def sources(mod):
    p = os.path.join(ROOT, 'src', mod.lower(), 'objs.txt')
    if not os.path.exists(p):
        return []
    return [os.path.join(mod.lower(), l.strip()) for l in open(p) if l.strip() and not l.startswith('#')]


for _k, _v in MODULES.items():
    _v['srcs'] = sources(_k)


def run(args, cwd, tool='CL.EXE'):
    rc, text = msvc.run(args, cwd, tool)
    out = '\n'.join(l for l in text.splitlines()
                    if l.strip() and not re.match(r'^[A-Z]:.*\.(c|cpp)$', l, re.I) and 'Creating library' not in l)
    return rc, out


def stub(mod):
    """Build a stand-in for an unwritten game DLL: its exports, empty."""
    d = os.path.join(OUT, 'stub', mod)
    os.makedirs(d, exist_ok=True)
    lib = os.path.join(d, os.path.splitext(MODULES[mod]['out'])[0] + '.lib')
    if os.path.exists(lib):
        return lib
    pe = PE(os.path.join(ROOT, 'orig', 'WINLEMM', ORIGNAME[mod]))
    c, cpp = [], []
    cxx = {  # the C++ exports' prototypes, from their decorated names
        '?CheckScript@@YGXH@Z': 'void __stdcall CheckScript(int a) {}',
        '?InitScript@@YGXPAD@Z': 'void __stdcall InitScript(char *a) {}',
        '?SoundInit@@YGXGG@Z': 'void __stdcall SoundInit(unsigned short a, unsigned short b) {}',
        '?SoundPlayFX@@YGXUtagPNT@@H@Z': 'void __stdcall SoundPlayFX(struct tagPNT a, int b) {}',
        '?SoundProcess@@YGXXZ': 'void __stdcall SoundProcess(void) {}',
        '?SoundQuit@@YGXXZ': 'void __stdcall SoundQuit(void) {}',
        '?SoundRender@@YGXXZ': 'void __stdcall SoundRender(void) {}',
        '?WaveIsEnabled@@YGHXZ': 'int __stdcall WaveIsEnabled(void) { return 0; }',
        '?WaveIsVolumeEnabled@@YGHXZ': 'int __stdcall WaveIsVolumeEnabled(void) { return 0; }',
        '?WaveSetVolume@@YGXH@Z': 'void __stdcall WaveSetVolume(int a) {}',
        '?WaveTestVolume@@YGXH@Z': 'void __stdcall WaveTestVolume(int a) {}',
    }
    for o, a, n in pe.exports:
        m = re.fullmatch(r'_(\w+)@(\d+)', n)
        if m:
            args = ', '.join(f'int a{i}' for i in range(int(m.group(2)) // 4)) or 'void'
            c.append(f'__declspec(dllexport) void __stdcall {m.group(1)}({args}) {{}}')
        elif n in cxx:
            cpp.append('__declspec(dllexport) ' + cxx[n])
        else:
            sys.exit(f'{mod}: no stub for export {n}')
    objs = []
    if c:
        open(os.path.join(d, 'stub.c'), 'w').write('\n'.join(c) + '\n')
        objs.append('stub.obj')
    if cpp:
        open(os.path.join(d, 'stubxx.cpp'), 'w').write('struct tagPNT { int x, y; };\n' + '\n'.join(cpp) + '\n')
        objs.append('stubxx.obj')
    for s in ['stub.c', 'stubxx.cpp']:
        if os.path.exists(os.path.join(d, s)):
            rc, out = run(['/nologo', '/c', s], d)
            if rc:
                sys.exit(out)
    rc, out = run(['/NOLOGO', '/DLL', f'/OUT:{MODULES[mod]["out"]}'] + objs +
                  ['LIBC.LIB', os.path.join(SYSLIBS, 'KERNEL32.LIB')], d, 'LINK.EXE')
    if not os.path.exists(lib):
        sys.exit(f'stub {mod}: {out}')
    return lib


def implib(mod):
    """Import library for a game DLL: from our matching build if the module is complete, else a stub."""
    m = MODULES[mod]
    built = os.path.join(OUT, mod.lower(), os.path.splitext(m['out'])[0] + '.lib')
    if m['srcs'] and os.path.exists(built):
        return built
    return stub(mod)


def build(mod, funcs=False):
    m = MODULES[mod]
    d = os.path.join(OUT, mod.lower())
    os.makedirs(d, exist_ok=True)
    objs = []
    for s in m['srcs']:
        src = os.path.join(ROOT, 'src', s)
        if s.endswith('.rc'):
            res = os.path.splitext(os.path.basename(s))[0] + '.res'
            rc, out = run(['/r', '/i', os.path.dirname(src), f'/fo{res}', src], d, 'RC.EXE')
            if rc or not os.path.exists(os.path.join(d, res)):
                print(out)
                print(f'{mod}: {s} failed to compile')
                return False
            objs.append(res)
            continue
        obj = os.path.splitext(os.path.basename(s))[0] + '.obj'
        rc, out = run(CFLAGS + [f'/Fo{obj}', src], d)
        if out:
            print(out)
        if rc:
            print(f'{mod}: {s} failed to compile')
            return False
        objs.append(obj)
        if funcs:
            subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'match.py'), ORIGNAME[mod], src,
                            os.path.join(d, obj)])
    libs = [os.path.join(SYSLIBS, 'LIBC.LIB')]
    for l in m['libs']:
        libs.append(implib(l) if l in MODULES else os.path.join(SYSLIBS, l + '.LIB'))
    flags = ['/NOLOGO', '/SUBSYSTEM:WINDOWS,4.0', '/MAP'] + ([] if m.get('exe') else DLLFLAGS) + m.get('flags', [])
    rc, out = run(flags + [f'/OUT:{m["out"]}'] + objs + libs, d, 'LINK.EXE')
    if out:
        print(out)
    if rc:
        print(f'{mod}: link failed')
        return False
    subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'cmpbin.py'), os.path.join(d, m['out']),
                    os.path.join(ROOT, 'orig', 'WINLEMM', ORIGNAME[mod])])
    return True


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    funcs = '--funcs' in sys.argv
    for tool in ['mkimplibs.py', 'mklibc.py']:
        subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'msvc', tool), SYSLIBS], check=True,
                       stdout=subprocess.DEVNULL)
    mods = [a.upper() for a in args] or [k for k, v in MODULES.items() if v['srcs']]
    ok = True
    for mod in mods:
        print(f'== {mod}', flush=True)
        ok &= build(mod, funcs)
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()

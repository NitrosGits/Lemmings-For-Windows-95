#!/usr/bin/env python3
"""Windows import libraries that give the same import hints as the ones the
original game was linked with (a 1995 Win32 SDK, newer than Visual C++ 2.0's).

Each is Visual C++ 2.0's own library with the hint of every function the game
imports patched to the value the original binaries carry.

  mkimplibs.py OUTDIR
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from pe import PE  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
VCLIB = os.path.join(os.environ.get('VCDIR', os.path.join(ROOT, 'build', 'vc22', 'MSVC20')), 'LIB')
GAME_DLLS = {'READDIB.DLL', 'RENDER.DLL', 'PROCESS.DLL', 'LOADLVL.DLL', 'VSMEM.DLL', 'FILEHAND.DLL', 'GAMEOVER.DLL'}
MODULES = ['LEMMINGS.EXE', 'VSMEM.DLL', 'READDIB.DLL', 'FILEHAND.DLL', 'GAMEOVER.DLL', 'LOADLVL.DLL', 'RENDER.DLL', 'PROCESS.DLL']


def members(d):
    """(header offset, data offset, size, name) of each archive member."""
    o = 8
    while o + 60 <= len(d):
        hdr = d[o:o + 60]
        size = int(hdr[48:58])
        yield o, o + 60, size, hdr[:16].rstrip()
        o += 60 + size + (size & 1)


def patch_hints(data, hints):
    """Set the hint in each import member's .idata$6 whose name is in hints."""
    d = bytearray(data)
    done = set()
    for ho, o, size, name in members(d):
        if name in (b'/', b'//'):
            continue
        machine, nsec = struct.unpack_from('<HH', d, o)
        if machine != 0x14c:
            continue
        optsz = struct.unpack_from('<H', d, o + 16)[0]
        for i in range(nsec):
            s = o + 20 + optsz + 40 * i
            if d[s:s + 8].rstrip(b'\0') != b'.idata$6':
                continue
            rsz, rptr = struct.unpack_from('<II', d, s + 16)
            sec = o + rptr
            nm = bytes(d[sec + 2:sec + rsz]).split(b'\0', 1)[0].decode('latin1')
            if nm in hints:
                struct.pack_into('<H', d, sec, hints[nm])
                done.add(nm)
    return bytes(d), done


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    per_dll = {}
    for m in MODULES:
        pe = PE(os.path.join(ROOT, 'orig', 'WINLEMM', m))
        for (dll, name), hint in pe.hints.items():
            if dll.upper() in GAME_DLLS:
                continue
            d = per_dll.setdefault(dll.upper(), {})
            if d.get(name, hint) != hint:
                sys.exit(f'{dll} {name}: hints {d[name]} and {hint} disagree')
            d[name] = hint
    for dll, hints in sorted(per_dll.items()):
        base = dll.rsplit('.', 1)[0]
        data, done = patch_hints(open(os.path.join(VCLIB, base + '.LIB'), 'rb').read(), hints)
        missing = set(hints) - done
        if missing:
            sys.exit(f'{base}.LIB: no member for {sorted(missing)}')
        open(os.path.join(outdir, base + '.LIB'), 'wb').write(data)
        print(f'{base}.LIB: {len(done)} hints set')


if __name__ == '__main__':
    main(sys.argv[1])

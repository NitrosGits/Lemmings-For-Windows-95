#!/usr/bin/env python3
"""List .text ranges that no function covers (undiscovered code or data in code)."""
import struct
import sys


def sections(path):
    d = open(path, 'rb').read()
    pe = struct.unpack_from('<I', d, 0x3c)[0]
    nsec = struct.unpack_from('<H', d, pe + 6)[0]
    optsz = struct.unpack_from('<H', d, pe + 20)[0]
    base = struct.unpack_from('<I', d, pe + 24 + 28)[0]
    out = []
    for i in range(nsec):
        o = pe + 24 + optsz + 40 * i
        name = d[o:o + 8].rstrip(b'\0').decode()
        vsz, va, rsz, rptr = struct.unpack_from('<IIII', d, o + 8)
        out.append((name, base + va, vsz, rptr, rsz))
    return d, base, out


def main(binpath, tsv):
    d, base, secs = sections(binpath)
    name, va, vsz, rptr, rsz = next(s for s in secs if s[0] == '.text')
    covered = bytearray(vsz)
    for line in open(tsv).readlines()[1:]:
        f = line.split('\t')
        a, e = int(f[0], 16), int(f[1], 16)
        for x in range(max(a, va), min(e + 1, va + vsz)):
            covered[x - va] = 1
    i = 0
    while i < vsz:
        if covered[i]:
            i += 1
            continue
        j = i
        while j < vsz and not covered[j]:
            j += 1
        blob = d[rptr + i:rptr + min(j, rsz)]
        if blob.strip(b'\x90\xcc\x00'):
            print(f'{va + i:#x}-{va + j:#x} {j - i:5d}  {blob[:12].hex()}')
        i = j


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])

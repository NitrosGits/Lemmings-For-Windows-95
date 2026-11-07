#!/usr/bin/env python3
"""Minimal COFF object reader (i386 .obj files from Visual C++)."""
import struct

IMAGE_REL_I386_DIR32 = 6
IMAGE_REL_I386_DIR32NB = 7
IMAGE_REL_I386_REL32 = 20


class Section:
    def __init__(self, idx, name, data, relocs, chars):
        self.idx = idx
        self.name = name
        self.data = data
        self.relocs = relocs      # list of (offset, symbol index, type)
        self.chars = chars


class Symbol:
    def __init__(self, idx, name, value, secnum, typ, cls, aux):
        self.idx = idx
        self.name = name
        self.value = value
        self.secnum = secnum
        self.type = typ
        self.cls = cls
        self.aux = aux

    @property
    def is_function(self):
        return (self.type >> 4) == 2


class Obj:
    def __init__(self, path):
        d = self.d = open(path, 'rb').read()
        (machine, nsec, ts, symptr, nsym, optsz, chars) = struct.unpack_from('<HHIIIHH', d, 0)
        strtab = symptr + 18 * nsym
        self.strtab = d[strtab:]
        self.sections = []
        for i in range(nsec):
            o = 20 + optsz + 40 * i
            raw = d[o:o + 8]
            if raw.startswith(b'/'):
                name = self._str(int(raw[1:].rstrip(b'\0')))
            else:
                name = raw.rstrip(b'\0').decode('latin1')
            (vsz, va, rsz, rptr, relptr, lnptr, nrel, nln, sc) = struct.unpack_from('<IIIIIIHHI', d, o + 8)
            data = d[rptr:rptr + rsz] if rptr else b'\0' * rsz
            relocs = []
            for k in range(nrel):
                ro, rs, rt = struct.unpack_from('<IIH', d, relptr + 10 * k)
                relocs.append((ro, rs, rt))
            self.sections.append(Section(i + 1, name, data, relocs, sc))
        self.symbols = {}
        i = 0
        while i < nsym:
            o = symptr + 18 * i
            raw = d[o:o + 8]
            if raw[:4] == b'\0\0\0\0':
                name = self._str(struct.unpack_from('<I', raw, 4)[0])
            else:
                name = raw.rstrip(b'\0').decode('latin1')
            value, secnum, typ, cls, naux = struct.unpack_from('<IhHBB', d, o + 8)
            aux = d[o + 18:o + 18 + 18 * naux]
            self.symbols[i] = Symbol(i, name, value, secnum, typ, cls, aux)
            i += 1 + naux

    def _str(self, off):
        return self.strtab[off:self.strtab.index(b'\0', off)].decode('latin1')

    def section(self, idx):
        return self.sections[idx - 1]

    def functions(self):
        """name -> (section, start, end) for every function defined here."""
        out = {}
        bysec = {}
        for s in self.symbols.values():
            if s.secnum > 0 and s.is_function:
                bysec.setdefault(s.secnum, []).append(s)
        for secnum, syms in bysec.items():
            sec = self.section(secnum)
            # other labels in the section bound a function too
            starts = sorted({s.value for s in self.symbols.values() if s.secnum == secnum and s.cls in (2, 3) and (s.is_function or s.value)})
            for s in syms:
                later = [v for v in starts if v > s.value]
                end = later[0] if later else len(sec.data)
                out[s.name] = (sec, s.value, end)
        return out

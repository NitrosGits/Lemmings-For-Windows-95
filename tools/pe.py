#!/usr/bin/env python3
"""Minimal PE32 reader: sections, imports (IAT slots), exports, base relocations."""
import struct


class PE:
    def __init__(self, path):
        self.path = path
        self.d = d = open(path, 'rb').read()
        pe = struct.unpack_from('<I', d, 0x3c)[0]
        self.nsec = struct.unpack_from('<H', d, pe + 6)[0]
        optsz = struct.unpack_from('<H', d, pe + 20)[0]
        opt = pe + 24
        self.base = struct.unpack_from('<I', d, opt + 28)[0]
        self.entry = self.base + struct.unpack_from('<I', d, opt + 16)[0]
        ndirs = struct.unpack_from('<I', d, opt + 92)[0]
        self.dirs = [struct.unpack_from('<II', d, opt + 96 + 8 * i) for i in range(ndirs)]
        self.sections = []
        for i in range(self.nsec):
            o = opt + optsz + 40 * i
            name = d[o:o + 8].rstrip(b'\0').decode()
            vsz, va, rsz, rptr = struct.unpack_from('<IIII', d, o + 8)
            chars = struct.unpack_from('<I', d, o + 36)[0]
            self.sections.append(dict(name=name, va=self.base + va, vsize=vsz, raw=rptr, rsize=rsz, chars=chars))
        self.hints = {}                     # (dll, name) -> hint
        self.imports = self._imports()      # iat_addr -> (dll, name)
        self.exports = self._exports()      # list of (ordinal, addr, name)
        self.relocs = self._relocs()        # set of addresses holding absolute pointers

    def sec(self, name):
        return next(s for s in self.sections if s['name'] == name)

    def sec_of(self, va):
        for s in self.sections:
            if s['va'] <= va < s['va'] + max(s['vsize'], s['rsize']):
                return s
        return None

    def off(self, va):
        s = self.sec_of(va)
        if s is None:
            return None
        o = va - s['va']
        if o >= s['rsize']:
            return None
        return s['raw'] + o

    def read(self, va, n):
        o = self.off(va)
        if o is None:
            return b'\0' * n
        s = self.sec_of(va)
        avail = s['raw'] + s['rsize'] - o
        b = self.d[o:o + min(n, avail)]
        return b + b'\0' * (n - len(b))

    def u32(self, va):
        return struct.unpack('<I', self.read(va, 4))[0]

    def u16(self, va):
        return struct.unpack('<H', self.read(va, 2))[0]

    def cstr(self, va, maxlen=4096):
        b = self.read(va, maxlen)
        return b.split(b'\0', 1)[0]

    def _imports(self):
        rva, size = self.dirs[1]
        out = {}
        if not rva:
            return out
        va = self.base + rva
        while True:
            olt, ts, fwd, name, iat = struct.unpack('<IIIII', self.read(va, 20))
            if not name and not iat:
                break
            dll = self.cstr(self.base + name).decode()
            lt = olt or iat
            i = 0
            while True:
                ent = self.u32(self.base + lt + 4 * i)
                if not ent:
                    break
                if ent & 0x80000000:
                    nm = f'ord{ent & 0xffff}'
                else:
                    nm = self.cstr(self.base + ent + 2).decode()
                    self.hints[(dll, nm)] = self.u16(self.base + ent)
                out[self.base + iat + 4 * i] = (dll, nm)
                i += 1
            va += 20
        return out

    def _exports(self):
        rva, size = self.dirs[0]
        if not rva:
            return []
        va = self.base + rva
        (chars, ts, maj, mnr, name, obase, nfun, nnames, afun, anames, aords) = struct.unpack('<IIHHIIIIIII', self.read(va, 40))
        names = {}
        for i in range(nnames):
            nrva = self.u32(self.base + anames + 4 * i)
            o = self.u16(self.base + aords + 2 * i)
            names[o] = self.cstr(self.base + nrva).decode()
        out = []
        for i in range(nfun):
            frva = self.u32(self.base + afun + 4 * i)
            if frva:
                out.append((obase + i, self.base + frva, names.get(i)))
        return out

    def _relocs(self):
        rva, size = self.dirs[5]
        out = set()
        if not rva:
            return out
        va = self.base + rva
        end = va + size
        while va < end:
            page, bsz = struct.unpack('<II', self.read(va, 8))
            if bsz < 8:
                break
            for i in range((bsz - 8) // 2):
                e = self.u16(va + 8 + 2 * i)
                if e >> 12 == 3:
                    out.add(self.base + page + (e & 0xfff))
            va += bsz
        return out

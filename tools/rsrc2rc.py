#!/usr/bin/env python3
"""Decompile a module's resources into an .rc file plus the binary files it
includes (cursors, icons, bitmaps):

  rsrc2rc.py MODULE.EXE OUT.rc [RESDIR]

Statements are written in the order of the resources' data in the original,
which is the order RC emitted them (the original .rc's statement order);
string tables come last, block by block in that same order. The binary files
go to RESDIR (default: res next to OUT.rc), named after their resources.

  rsrc2rc.py --cmp ORIGINAL BUILT

compares the two files' resources one by one.
"""
import os
import struct
import sys

TYPES = {1: 'CURSOR', 2: 'BITMAP', 3: 'ICON', 4: 'MENU', 5: 'DIALOG', 6: 'STRING', 9: 'ACCELERATORS',
         10: 'RCDATA', 12: 'GROUP_CURSOR', 14: 'GROUP_ICON', 16: 'VERSION'}


def resources(path):
    """[(type, name, lang, data, rva)] in data order."""
    d = open(path, 'rb').read()
    pe = struct.unpack_from('<I', d, 0x3c)[0]
    n = struct.unpack_from('<H', d, pe + 6)[0]
    opt = struct.unpack_from('<H', d, pe + 20)[0]
    rs = None
    for i in range(n):
        name, vs, va, rsz, ro = struct.unpack_from('<8sIIII', d, pe + 24 + opt + 40 * i)
        if name.rstrip(b'\0') == b'.rsrc':
            rs = (va, ro)
    if not rs:
        return []
    va, base = rs

    def name(v):
        if v & 0x80000000:
            o = base + (v & 0x7fffffff)
            l = struct.unpack_from('<H', d, o)[0]
            return d[o + 2:o + 2 + 2 * l].decode('utf-16le')
        return v

    out = []

    def walk(off, path):
        o = base + off
        nn, ni = struct.unpack_from('<HH', d, o + 12)
        for i in range(nn + ni):
            nm, ptr = struct.unpack_from('<II', d, o + 16 + 8 * i)
            p = path + [name(nm)]
            if ptr & 0x80000000:
                walk(ptr & 0x7fffffff, p)
            else:
                rva, sz, cp, _ = struct.unpack_from('<IIII', d, base + ptr)
                out.append((p[0], p[1], p[2], d[base + rva - va:base + rva - va + sz], rva))

    walk(0, [])
    out.sort(key=lambda r: r[4])
    return out


def rcname(n):
    return str(n) if isinstance(n, int) else n


def rcstr(s, nul=False):
    """A string as an RC literal (cp1252, octal escapes for the rest)."""
    r = '"'
    for c in s:
        if c == '"':
            r += '""'
        elif c == '\\':
            r += '\\\\'
        elif c == '\t':
            r += '\\t'
        elif c == '\n':
            r += '\\n'
        elif c == '\r':
            r += '\\r'
        elif c == '\0':
            r += '\\0'
        elif 32 <= ord(c) < 127:
            r += c
        else:
            b = c.encode('cp1252')
            r += ''.join('\\%03o' % x for x in b)
    return r + '"'


def wstr(data, o):
    """A NUL-terminated UTF-16 string or 0xffff ordinal at o: (value, new o)."""
    w = struct.unpack_from('<H', data, o)[0]
    if w == 0xffff:
        return struct.unpack_from('<H', data, o + 2)[0], o + 4
    e = o
    while struct.unpack_from('<H', data, e)[0]:
        e += 2
    return data[o:e].decode('utf-16le'), e + 2


WS = [(0x80000000, 'WS_POPUP'), (0x40000000, 'WS_CHILD'), (0x20000000, 'WS_MINIMIZE'),
      (0x10000000, 'WS_VISIBLE'), (0x08000000, 'WS_DISABLED'), (0x04000000, 'WS_CLIPSIBLINGS'),
      (0x02000000, 'WS_CLIPCHILDREN'), (0x01000000, 'WS_MAXIMIZE'), (0x00C00000, 'WS_CAPTION'),
      (0x00800000, 'WS_BORDER'), (0x00400000, 'WS_DLGFRAME'), (0x00200000, 'WS_VSCROLL'),
      (0x00100000, 'WS_HSCROLL'), (0x00080000, 'WS_SYSMENU'), (0x00040000, 'WS_THICKFRAME'),
      (0x00020000, 'WS_GROUP'), (0x00010000, 'WS_TABSTOP')]


def hexstyle(v):
    return '0x%xL' % v


CLASSES = {0x80: 'Button', 0x81: 'Edit', 0x82: 'Static', 0x83: 'ListBox', 0x84: 'ScrollBar', 0x85: 'ComboBox'}


def dialog(name, data):
    style, exstyle, n, x, y, cx, cy = struct.unpack_from('<IIHhhhh', data, 0)
    o = 18
    menu, o = wstr(data, o)
    cls, o = wstr(data, o)
    title, o = wstr(data, o)
    font = None
    if style & 0x40:            # DS_SETFONT
        pt = struct.unpack_from('<H', data, o)[0]
        face, o = wstr(data, o + 2)
        font = (pt, face)
    L = [f'{rcname(name)} DIALOG {x}, {y}, {cx}, {cy}']
    # RC adds WS_CAPTION for CAPTION and DS_SETFONT for FONT: take them out and
    # let RC put them back, or write NOT where it would add what isn't there.
    st = style
    extra = []
    if title:
        if (style & 0xC00000) != 0xC00000:
            extra.append('NOT WS_CAPTION')
    L.append('STYLE ' + hexstyle(st) + (' | ' + ' | '.join(extra) if extra else ''))
    if exstyle:
        L.append('EXSTYLE ' + hexstyle(exstyle))
    if title or title == '':
        if title:
            L.append('CAPTION ' + rcstr(title))
    if menu:
        L.append('MENU ' + rcname(menu))
    if cls:
        L.append('CLASS ' + (rcstr(cls) if isinstance(cls, str) else str(cls)))
    if font:
        L.append(f'FONT {font[0]}, {rcstr(font[1])}')
    L.append('BEGIN')
    for i in range(n):
        o = (o + 3) & ~3
        st, ex, x, y, cx, cy, cid = struct.unpack_from('<IIhhhhH', data, o)
        o += 18
        c, o = wstr(data, o)
        t, o = wstr(data, o)
        extra_n = struct.unpack_from('<H', data, o)[0]
        o += 2 + extra_n
        if extra_n:
            raise ValueError(f'dialog {name}: control creation data not supported')
        cname = rcstr(CLASSES[c]) if isinstance(c, int) else rcstr(c)
        text = rcstr(t) if isinstance(t, str) else str(t)
        # CONTROL always gets WS_CHILD | WS_VISIBLE added
        s = hexstyle(st)
        if not st & 0x10000000:
            s += ' | NOT WS_VISIBLE'
        line = f'    CONTROL {text}, {cid if cid < 0x8000 else cid - 0x10000}, {cname}, {s}, {x}, {y}, {cx}, {cy}'
        if ex:
            line += ', ' + hexstyle(ex)
        L.append(line)
    L.append('END')
    return '\n'.join(L)


def menu(name, data):
    ver, hsize = struct.unpack_from('<HH', data, 0)
    o = 4 + hsize
    L = [f'{rcname(name)} MENU']

    def items(o, depth):
        ind = '    ' * depth
        L.append(ind[4:] + 'BEGIN')
        while True:
            flags = struct.unpack_from('<H', data, o)[0]
            o += 2
            if flags & 0x10:            # MF_POPUP
                text, o = wstr(data, o)
                opts = menuopts(flags)
                L.append(f'{ind}POPUP {rcstr(text)}' + opts)
                o = items(o, depth + 1)
            else:
                mid = struct.unpack_from('<H', data, o)[0]
                o += 2
                text, o = wstr(data, o)
                if flags & ~0x80 == 0 and mid == 0 and text == '':
                    L.append(f'{ind}MENUITEM SEPARATOR')
                else:
                    L.append(f'{ind}MENUITEM {rcstr(text)}, {mid}' + menuopts(flags))
            if flags & 0x80:            # MF_END
                break
        L.append(ind[4:] + 'END')
        return o

    items(o, 1)
    return '\n'.join(L)


def menuopts(flags):
    o = []
    for bit, nm in ((1, 'GRAYED'), (2, 'INACTIVE'), (8, 'CHECKED'), (0x20, 'MENUBARBREAK'), (0x40, 'MENUBREAK'),
                    (0x4000, 'HELP')):
        if flags & bit:
            o.append(nm)
    left = flags & ~(0x80 | 0x10 | 1 | 2 | 8 | 0x20 | 0x40 | 0x4000)
    if left:
        raise ValueError('menu flags %x' % flags)
    return (', ' + ', '.join(o)) if o else ''


def accel(name, data):
    L = [f'{rcname(name)} ACCELERATORS', 'BEGIN']
    for o in range(0, len(data), 8):
        flags, key, cid = struct.unpack_from('<HHH', data, o)
        opts = ['VIRTKEY' if flags & 1 else 'ASCII']
        for bit, nm in ((2, 'NOINVERT'), (4, 'SHIFT'), (8, 'CONTROL'), (0x10, 'ALT')):
            if flags & bit:
                opts.append(nm)
        L.append(f'    {key}, {cid}, ' + ', '.join(opts))
    L.append('END')
    return '\n'.join(L)


def version(name, data):
    def node(o):
        ln, vln, typ = struct.unpack_from('<HHH', data, o)
        key, p = wstr(data, o + 6)
        p = (p + 3) & ~3
        if typ == 1:
            val = data[p:p + 2 * vln].decode('utf-16le')
            p += 2 * vln
        else:
            val = data[p:p + vln]
            p += vln
        p = (p + 3) & ~3
        kids = []
        while p < o + ln:
            k, p2 = node(p)
            kids.append(k)
            p = (p2 + 3) & ~3
        return (key, typ, val, kids), o + ln

    (key, typ, val, kids), _ = node(0)
    (sig, sv, fvm, fvl, pvm, pvl, fmask, fflags, fos, ftype, fsub, dm, dl) = struct.unpack('<13I', val[:52])
    L = [f'{rcname(name)} VERSIONINFO',
         f'FILEVERSION {fvm >> 16},{fvm & 0xffff},{fvl >> 16},{fvl & 0xffff}',
         f'PRODUCTVERSION {pvm >> 16},{pvm & 0xffff},{pvl >> 16},{pvl & 0xffff}',
         f'FILEFLAGSMASK 0x{fmask:x}L', f'FILEFLAGS 0x{fflags:x}L', f'FILEOS 0x{fos:x}L',
         f'FILETYPE 0x{ftype:x}L', f'FILESUBTYPE 0x{fsub:x}L', 'BEGIN']

    def emit(n, depth):
        k, t, v, ks = n
        ind = '    ' * depth
        if ks or (not v and t == 1 and depth < 3):
            L.append(f'{ind}BLOCK {rcstr(k)}')
            L.append(f'{ind}BEGIN')
            for c in ks:
                emit(c, depth + 1)
            L.append(f'{ind}END')
        elif t == 1:
            L.append(f'{ind}VALUE {rcstr(k)}, {rcstr(v)}')
        else:
            words = struct.unpack('<%dH' % (len(v) // 2), v)
            L.append(f'{ind}VALUE {rcstr(k)}, ' + ', '.join('0x%x' % w for w in words))

    for c in kids:
        emit(c, 1)
    L.append('END')
    return '\n'.join(L)


def main(path, out, resdir=None):
    res = resources(path)
    resdir = resdir or os.path.join(os.path.dirname(out), 'res')
    rel = os.path.relpath(resdir, os.path.dirname(out))
    os.makedirs(resdir, exist_ok=True)
    byid = {(r[0], r[1]): r for r in res}
    parts = ['LANGUAGE %d, %d' % (res[0][2] & 0x3ff, res[0][2] >> 10)] if res else []
    strings = []
    for typ, name, lang, data, rva in res:
        t = TYPES.get(typ, typ)
        if t in ('CURSOR', 'ICON'):
            continue                    # written with their group
        if t in ('GROUP_CURSOR', 'GROUP_ICON'):
            cur = t == 'GROUP_CURSOR'
            _, kind, n = struct.unpack_from('<HHH', data, 0)
            imgs = []
            for i in range(n):
                e = data[6 + 14 * i:20 + 14 * i]
                if cur:
                    w, h, planes, bits, size, iid = struct.unpack('<HHHHIH', e)
                    img = byid[(1, iid)][3]
                    hx, hy = struct.unpack_from('<hh', img, 0)
                    imgs.append((struct.pack('<BBBBhh', w, h // 2, 0, 0, hx, hy), img[4:]))
                else:
                    w, h, cc, rsv, planes, bits, size, iid = struct.unpack('<BBBBHHIH', e)
                    img = byid[(3, iid)][3]
                    imgs.append((struct.pack('<BBBBHH', w, h, cc, rsv, planes, bits), img))
            f = struct.pack('<HHH', 0, 2 if cur else 1, n)
            off = 6 + 16 * n
            for hdr, img in imgs:
                f += hdr + struct.pack('<II', len(img), off)
                off += len(img)
            f += b''.join(img for _, img in imgs)
            fn = f'{rcname(name).lower()}.{"cur" if cur else "ico"}'
            open(os.path.join(resdir, fn), 'wb').write(f)
            parts.append(f'{rcname(name)} {"CURSOR" if cur else "ICON"} "{rel}/{fn}"')
        elif t == 'BITMAP':
            hsz, = struct.unpack_from('<I', data, 0)
            bits, clr = struct.unpack_from('<H', data, 14)[0], struct.unpack_from('<I', data, 32)[0]
            ncol = clr or (1 << bits if bits <= 8 else 0)
            offbits = 14 + hsz + 4 * ncol
            fn = f'{rcname(name).lower()}.bmp'
            open(os.path.join(resdir, fn), 'wb').write(b'BM' + struct.pack('<IHHI', 14 + len(data), 0, 0, offbits) + data)
            parts.append(f'{rcname(name)} BITMAP "{rel}/{fn}"')
        elif t == 'DIALOG':
            parts.append(dialog(name, data))
        elif t == 'MENU':
            parts.append(menu(name, data))
        elif t == 'ACCELERATORS':
            parts.append(accel(name, data))
        elif t == 'VERSION':
            parts.append(version(name, data))
        elif t == 'RCDATA':
            text = data.decode('cp1252')
            parts.append(f'{rcname(name)} RCDATA\nBEGIN\n    {rcstr(text)}\nEND')
        elif t == 'STRING':
            strings.append((name, data))
        else:
            raise ValueError(f'resource type {typ} not supported')
    if strings:
        L = ['STRINGTABLE', 'BEGIN']
        for block, data in strings:
            o = 0
            for i in range(16):
                n = struct.unpack_from('<H', data, o)[0]
                s = data[o + 2:o + 2 + 2 * n].decode('utf-16le')
                o += 2 + 2 * n
                if n:
                    L.append(f'    {(block - 1) * 16 + i}, {rcstr(s)}')
        L.append('END')
        parts.append('\n'.join(L))
    open(out, 'w', encoding='cp1252', newline='\r\n').write('\n\n'.join(parts) + '\n')


def cmp(a, b):
    ra, rb = resources(a), resources(b)
    ka = {(r[0], r[1]): r for r in ra}
    kb = {(r[0], r[1]): r for r in rb}
    bad = 0
    for k, r in ka.items():
        t = TYPES.get(k[0], k[0])
        if k not in kb:
            print(f'{t} {k[1]}: missing')
            bad += 1
        elif kb[k][3] != r[3]:
            x, y = r[3], kb[k][3]
            i = next((i for i in range(min(len(x), len(y))) if x[i] != y[i]), min(len(x), len(y)))
            print(f'{t} {k[1]}: differs at +0x{i:x} (sizes {len(x)} vs {len(y)})')
            bad += 1
    for k in kb:
        if k not in ka:
            print(f'{TYPES.get(k[0], k[0])} {k[1]}: extra')
            bad += 1
    order = [(r[0], r[1]) for r in ra] == [(r[0], r[1]) for r in rb]
    print(f'{len(ra) - bad}/{len(ra)} resources match; data order {"matches" if order else "differs"}')


if __name__ == '__main__':
    if sys.argv[1] == '--cmp':
        cmp(sys.argv[2], sys.argv[3])
    else:
        main(*sys.argv[1:])

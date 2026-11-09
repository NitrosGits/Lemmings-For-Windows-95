#!/usr/bin/env python3
"""Turn a Windows resource script into C tables for the native SDL3 build.

    rc2c.py src/lemmings/lemmings.rc build/native/gen/lemmings_res.c

Reads the subset of the .rc language that lemmings.rc uses: DIALOG (STYLE,
CAPTION, FONT, CONTROL lines), MENU (POPUP, MENUITEM, SEPARATOR),
ACCELERATORS, STRINGTABLE, VERSIONINFO (StringFileInfo values), and BITMAP /
CURSOR / ICON files, which are embedded byte for byte. Preprocessor lines are
dropped, keeping the #else branch of an #if (the RC.EXE spelling). Strings
keep their Windows-1252 bytes. The tables' types are in port/src/port_res.h.
"""
import os
import re
import sys

TOKEN = re.compile(r'"(?:[^"]|"")*"|[A-Za-z_][A-Za-z0-9_]*|-?0[xX][0-9a-fA-F]+L?|-?[0-9]+L?|[,|]')


def strip_preprocessor(text):
    """Drops #if/#else/#endif lines, keeping the lines of the #else branch."""
    out, stack = [], []
    for line in text.splitlines():
        s = line.strip()
        if s.startswith('#if'):
            stack.append(False)          # skip the #if branch
            continue
        if s.startswith('#else'):
            stack[-1] = True
            continue
        if s.startswith('#endif'):
            stack.pop()
            continue
        if s.startswith('#') or s.startswith('//'):
            continue
        if all(stack):
            out.append(line)
    return out


def unquote(tok):
    """A .rc string literal to bytes (octal and C escapes, "" for a quote)."""
    s = tok[1:-1].replace('""', '"')
    out = bytearray()
    i = 0
    while i < len(s):
        c = s[i]
        if c == '\\' and i + 1 < len(s):
            n = s[i + 1]
            if n in '01234567':
                j = i + 1
                while j < len(s) and j < i + 4 and s[j] in '01234567':
                    j += 1
                out.append(int(s[i + 1:j], 8) & 0xff)
                i = j
                continue
            out.append({'n': 10, 't': 9, 'r': 13, '\\': 92, '"': 34, 'a': 7}.get(n, ord(n)))
            i += 2
            continue
        out += c.encode('cp1252')
        i += 1
    return bytes(out)


def cstr(b):
    """bytes to a C string literal (stops at the first NUL, as the resource does)."""
    if b is None:
        return 'NULL'
    b = b.split(b'\0')[0]
    out = '"'
    for x in b:
        if x == 34 or x == 92:
            out += '\\' + chr(x)
        elif 32 <= x < 127 and x != 63:
            out += chr(x)
        else:
            out += '\\%03o' % x
    return out + '"'


def number(tok):
    return int(tok.rstrip('L'), 0)


def expr(toks, i):
    """A number or NAME, possibly ORed: returns (value, next index)."""
    v = number(toks[i])
    i += 1
    while i < len(toks) and toks[i] == '|':
        v |= number(toks[i + 1])
        i += 2
    return v & 0xffffffff, i


class Parser:
    def __init__(self, path):
        self.dir = os.path.dirname(path)
        lines = strip_preprocessor(open(path, encoding='cp1252').read())
        self.toks = []
        for line in lines:
            self.toks += TOKEN.findall(line)
        self.i = 0
        self.dialogs, self.menus, self.accels, self.strings, self.files, self.version = [], [], [], [], [], []

    def peek(self, k=0):
        return self.toks[self.i + k] if self.i + k < len(self.toks) else None

    def take(self):
        t = self.toks[self.i]
        self.i += 1
        return t

    def expect(self, t):
        got = self.take()
        if got != t:
            raise SyntaxError('expected %s, got %s at token %d' % (t, got, self.i))

    def comma(self):
        if self.peek() == ',':
            self.i += 1

    def name(self, tok):
        """A resource name: (string or None, number or None)."""
        if re.match(r'-?[0-9]', tok):
            return None, number(tok)
        return tok.upper(), None

    def parse(self):
        while self.i < len(self.toks):
            t = self.take()
            if t == 'LANGUAGE':
                self.take(); self.comma(); self.take()
            elif t == 'STRINGTABLE':
                self.stringtable()
            else:
                kind = self.take()
                if kind in ('BITMAP', 'CURSOR', 'ICON'):
                    path = unquote(self.take()).decode('cp1252')
                    data = open(os.path.join(self.dir, path), 'rb').read()
                    self.files.append((kind, self.name(t), data))
                elif kind == 'DIALOG':
                    self.dialog(self.name(t))
                elif kind == 'MENU':
                    self.menus.append((self.name(t), self.menu()))
                elif kind == 'ACCELERATORS':
                    self.accelerators(self.name(t))
                elif kind == 'RCDATA':
                    self.skip_block()
                elif kind == 'VERSIONINFO':
                    self.versioninfo()
                else:
                    raise SyntaxError('unknown resource type ' + kind)

    def skip_block(self):
        while self.take() != 'BEGIN':
            pass
        depth = 1
        while depth:
            t = self.take()
            depth += t == 'BEGIN'
            depth -= t == 'END'

    def stringtable(self):
        self.expect('BEGIN')
        while self.peek() != 'END':
            n = number(self.take())
            self.comma()
            self.strings.append((n, unquote(self.take())))
        self.take()

    def dialog(self, name):
        x = number(self.take()); self.comma()
        y = number(self.take()); self.comma()
        w = number(self.take()); self.comma()
        h = number(self.take())
        style, caption, font = 0, None, (8, b'MS Sans Serif')
        while self.peek() != 'BEGIN':
            t = self.take()
            if t == 'STYLE':
                style, self.i = expr(self.toks, self.i)
            elif t == 'CAPTION':
                caption = unquote(self.take())
            elif t == 'FONT':
                size = number(self.take()); self.comma()
                font = (size, unquote(self.take()))
            else:
                raise SyntaxError('dialog option ' + t)
        self.take()
        controls = []
        while self.peek() != 'END':
            self.expect('CONTROL')
            text = unquote(self.take()); self.comma()
            cid = number(self.take()); self.comma()
            cls = unquote(self.take()); self.comma()
            cstyle, self.i = expr(self.toks, self.i); self.comma()
            cx = number(self.take()); self.comma()
            cy = number(self.take()); self.comma()
            cw = number(self.take()); self.comma()
            ch = number(self.take())
            controls.append((text, cid, cls, cstyle, cx, cy, cw, ch))
        self.take()
        self.dialogs.append((name, style, x, y, w, h, caption, font, controls))

    def menu(self):
        self.expect('BEGIN')
        items = []
        while self.peek() != 'END':
            t = self.take()
            if t == 'POPUP':
                text = unquote(self.take())
                items.append((text, 0, 'popup', self.menu()))
            elif t == 'MENUITEM':
                if self.peek() == 'SEPARATOR':
                    self.take()
                    items.append((None, 0, 'separator', []))
                else:
                    text = unquote(self.take()); self.comma()
                    items.append((text, number(self.take()), 'item', []))
            else:
                raise SyntaxError('menu item ' + t)
        self.take()
        return items

    def accelerators(self, name):
        self.expect('BEGIN')
        table = []
        while self.peek() != 'END':
            key = self.take()
            key = unquote(key)[0] if key.startswith('"') else number(key)
            self.comma()
            cmd = number(self.take())
            flags = 0
            while self.peek() == ',':
                self.take()
                f = self.take()
                flags |= {'VIRTKEY': 1, 'NOINVERT': 2, 'SHIFT': 4, 'CONTROL': 8, 'ALT': 0x10, 'ASCII': 0}[f]
            table.append((key, cmd, flags))
        self.take()
        self.accels.append((name, table))

    def versioninfo(self):
        while self.peek() != 'BEGIN':
            self.take()
        self.take()
        depth, lang = 1, None
        while depth:
            t = self.take()
            if t == 'BEGIN':
                depth += 1
            elif t == 'END':
                depth -= 1
            elif t == 'BLOCK':
                lang = unquote(self.take()).decode()
            elif t == 'VALUE':
                key = unquote(self.take()).decode()
                vals = []
                while self.peek() == ',':
                    self.take()
                    vals.append(self.take())
                if vals and vals[0].startswith('"'):
                    self.version.append((lang, key, unquote(vals[0])))


def emit(p, out):
    w = out.write
    w('/* Generated by tools/port/rc2c.py from the game\'s resource script: do not edit. */\n')
    w('#include "port_res.h"\n\n')

    def nm(n):
        return '%s, %d' % (cstr(n[0].encode()) if n[0] else 'NULL', n[1] if n[1] is not None else -1)

    for k, (kind, name, data) in enumerate(p.files):
        w('static const unsigned char file%d[%d] = {' % (k, len(data)))
        for j, b in enumerate(data):
            w(('\n    ' if j % 16 == 0 else '') + '0x%02x,' % b)
        w('\n};\n')
    w('const PortResFile port_res_files[] = {\n')
    for k, (kind, name, data) in enumerate(p.files):
        w('    {%s, %s, file%d, %d},\n' % (cstr(kind.encode()), nm(name), k, len(data)))
    w('    {NULL, NULL, -1, NULL, 0}\n};\n\n')

    w('const PortResString port_res_strings[] = {\n')
    for n, s in sorted(p.strings):
        w('    {%d, %s},\n' % (n, cstr(s)))
    w('    {0, NULL}\n};\n\n')

    for k, d in enumerate(p.dialogs):
        w('static const PortResControl dlg%d_controls[] = {\n' % k)
        for (text, cid, cls, style, x, y, cw, ch) in d[8]:
            w('    {%s, %d, %s, 0x%08xu, %d, %d, %d, %d},\n' % (cstr(text), cid, cstr(cls), style, x, y, cw, ch))
        w('};\n')
    w('const PortResDialog port_res_dialogs[] = {\n')
    for k, (name, style, x, y, wd, h, caption, font, controls) in enumerate(p.dialogs):
        w('    {%s, 0x%08xu, %d, %d, %d, %d, %s, %d, %s, %d, dlg%d_controls},\n' %
          (nm(name), style, x, y, wd, h, cstr(caption), font[0], cstr(font[1]), len(controls), k))
    w('    {NULL, -1, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, NULL}\n};\n\n')

    count = [0]

    def menu_items(items):
        subs = [menu_items(it[3]) if it[2] == 'popup' else None for it in items]
        k = count[0]
        count[0] += 1
        w('static const PortResMenuItem menu%d[] = {\n' % k)
        for it, sub in zip(items, subs):
            flags = {'popup': 'MF_POPUP', 'separator': 'MF_SEPARATOR', 'item': '0'}[it[2]]
            w('    {%s, %d, %s, %d, %s},\n' % (cstr(it[0]), it[1], flags, len(it[3]) if sub is not None else 0,
                                            'menu%d' % sub if sub is not None else 'NULL'))
        w('};\n')
        return k
    tops = [(name, len(items), menu_items(items)) for name, items in p.menus]
    w('const PortResMenu port_res_menus[] = {\n')
    for name, n, k in tops:
        w('    {%s, %d, menu%d},\n' % (nm(name), n, k))
    w('    {NULL, -1, 0, NULL}\n};\n\n')

    for k, (name, table) in enumerate(p.accels):
        w('static const PortResAccel accel%d[] = {\n' % k)
        for key, cmd, flags in table:
            w('    {0x%02x, %d, %d},\n' % (flags, key, cmd))
        w('};\n')
    w('const PortResAccelTable port_res_accels[] = {\n')
    for k, (name, table) in enumerate(p.accels):
        w('    {%s, %d, accel%d},\n' % (nm(name), len(table), k))
    w('    {NULL, -1, 0, NULL}\n};\n\n')

    w('const PortResVersion port_res_version[] = {\n')
    for lang, key, value in p.version:
        w('    {%s, %s, %s},\n' % (cstr(lang.encode()), cstr(key.encode()), cstr(value)))
    w('    {NULL, NULL, NULL}\n};\n')


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    p = Parser(sys.argv[1])
    p.parse()
    os.makedirs(os.path.dirname(os.path.abspath(sys.argv[2])), exist_ok=True)
    with open(sys.argv[2], 'w') as out:
        emit(p, out)


if __name__ == '__main__':
    main()

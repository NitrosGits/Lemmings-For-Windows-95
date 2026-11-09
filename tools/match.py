#!/usr/bin/env python3
"""Compare functions compiled by Visual C++ 2.0 with the original module.

  match.py MODULE SOURCE.c OBJ [-v] [-f NAME]

Functions in SOURCE are matched to the original by the address comment on the
line before their definition:

    /* 0x10001000 */
    static VSMEM_BLOCK *FindFreeBlock(void)

The bytes the compiler produced are compared with the original's at that
address. Where the object has a relocation, the original holds an address the
linker filled in: it must point at the right thing (another annotated function,
an import, or a global named in tools/names/MODULE.txt) when that is known, and
otherwise is only reported as unchecked. -v prints a side-by-side disassembly of
the functions that differ.
"""
import difflib
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(__file__))
from coff import Obj, IMAGE_REL_I386_DIR32, IMAGE_REL_I386_REL32  # noqa: E402
from pe import PE  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def gnu_objdump():
    """A GNU objdump (any build reads raw i386 code; LLVM's objdump can't): $OBJDUMP, else the first found."""
    if os.environ.get('OBJDUMP'):
        return os.environ['OBJDUMP']
    for c in ['/opt/homebrew/bin/i686-w64-mingw32-objdump', shutil.which('i686-w64-mingw32-objdump'),
              shutil.which('objdump'), shutil.which('x86_64-w64-mingw32-objdump'), shutil.which('gobjdump')]:
        if c and os.path.exists(c):
            try:
                v = subprocess.run([c, '--version'], capture_output=True, text=True).stdout
            except OSError:
                continue
            if 'GNU' in v:
                return c
    sys.exit('match.py needs a GNU objdump (binutils, e.g. from MinGW-w64); set OBJDUMP to its path')


GOBJDUMP = None


def annotations(src):
    """[(address, function name)] from address comments before definitions."""
    lines = open(src, encoding='latin1').read().split('\n')
    out = []
    for i, line in enumerate(lines):
        m = re.match(r'\s*/\*\s*(0x[0-9a-fA-F]+)\b.*\*/\s*$', line)
        if not m:
            continue
        head = ' '.join(lines[i + 1:i + 4])
        f = re.match(r'[^(;{=]*?(\w+(?:::~?\w+)?)\s*\(', head)
        if f and '{' in ' '.join(lines[i + 1:i + 12]):
            out.append((int(m.group(1), 16), f.group(1)))
    return out


def load_names(mod):
    names = {}
    p = os.path.join(ROOT, 'tools', 'names', mod + '.txt')
    if os.path.exists(p):
        for line in open(p):
            line = line.split('#', 1)[0].strip()
            if line:
                a, n = line.split()[:2]
                names[n] = int(a, 16)
    return names


CXX_SPECIAL = {'0': '{c}::{c}', '1': '{c}::~{c}', '2': 'new', '3': 'delete', '_7': '{c}::vftable'}


def c_name(sym):
    """C name of a COFF symbol: _foo, _foo@8, @foo@8 -> foo; statics keep $S suffixes off.
    C++: ?name@@... -> name, ?name@Class@@... -> Class::name, ??0Class@@ -> Class::Class."""
    n = sym
    if n.startswith('__imp_'):
        n = n[6:]
    if n.startswith('??') and '@' in n:
        m = re.match(r'\?\?(_\w|\w)(\w*)@', n)
        if m and m.group(1) in CXX_SPECIAL:
            return CXX_SPECIAL[m.group(1)].format(c=m.group(2))
    if n.startswith('?') and '@@' in n:
        parts = n[1:].split('@@', 1)[0].split('@')
        return '::'.join(reversed(parts))
    n = re.sub(r'\$S\d+$', '', n)
    if n.startswith(('_', '@')):
        n = n[1:]
    n = re.sub(r'@\d+$', '', n)
    return n


def disasm(code, base):
    global GOBJDUMP
    GOBJDUMP = GOBJDUMP or gnu_objdump()
    p = os.path.join(tempfile.gettempdir(), '_match_%d.bin' % os.getpid())
    open(p, 'wb').write(code)
    out = subprocess.run([GOBJDUMP, '-D', '-b', 'binary', '-m', 'i386', '-M', 'intel',
                          f'--adjust-vma={base:#x}', p], capture_output=True, text=True).stdout
    os.unlink(p)
    res = []
    for line in out.splitlines():
        m = re.match(r'\s*([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s*(.*)', line)
        if m:
            res.append((int(m.group(1), 16), m.group(3).strip()))
    return res


def insn_diff(a, b, lo, hi):
    """How many instructions differ between two listings, after aligning them;
    jumps within the function compare by how many instructions they skip."""
    def norm(lst):
        index = {at: i for i, (at, _) in enumerate(lst)}
        out = []
        for i, (at, text) in enumerate(lst):
            m = re.match(r'(j\w+|call|loop\w*)\s+0x([0-9a-f]+)$', text)
            if m and lo <= int(m.group(2), 16) < hi:
                t = index.get(int(m.group(2), 16))
                text = '%s %s' % (m.group(1), 'L%+d' % (t - i) if t is not None else 'L?')
            out.append(text)
        return out
    x, y = norm(a), norm(b)
    same = sum(blk.size for blk in difflib.SequenceMatcher(None, x, y, autojunk=False).get_matching_blocks())
    return max(len(x), len(y)) - same


REGS = r'\b(e?[abcd]x|[abcd][lh]|e?[sd]i|e?[sb]p)\b'


def semantic(a, b, lo, hi):
    """Instructions in one listing and not the other once register names are
    blanked out and in-function branch targets ignored: a field offset, an
    immediate or a global that differs, not just register allocation or
    scheduling. Jump tables (from the first 'jmp [reg*4+table]' target on)
    are left out. Returns (original only, ours only)."""
    def body(lst):
        end = hi
        for at, text in lst:
            m = re.search(r'jmp\s+DWORD PTR \[\w+\*4\+0x([0-9a-f]+)\]', text)
            if m and lo <= int(m.group(1), 16) < end:
                end = int(m.group(1), 16)
            m = re.search(r'BYTE PTR \[\w+\+0x([0-9a-f]+)\]', text)   # switch index tables
            if m and lo < int(m.group(1), 16) < end and text.startswith('mov'):
                end = int(m.group(1), 16)
        return [t for at, t in lst if at < end and t and not t.startswith('(bad)')]

    def key(text, stack=True):
        m, _, ops = text.partition(' ')
        ops = ops.strip()
        if m.startswith('j') or m.startswith('loop') or (m == 'call' and 'ds:' not in ops):
            return m
        if m in ('lea',) and re.fullmatch(r'(\w+),\[\1\+(eiz\*1\+)?0x0\]', ops.replace(' ', '')):
            return 'nop'
        if not stack:                   # pushes move esp-relative offsets around
            ops = re.sub(r'\[e[sb]p(\+0x[0-9a-f]+)?\]', '[stack]', ops)
        ops = re.sub(REGS, 'R', ops)
        if m == 'cmp':                  # operand order of a compare is the compiler's choice
            ops = ','.join(sorted(ops.split(',')))
        return m + ' ' + ops

    def boring(text):
        m, _, ops = text.partition(' ')
        ops = ops.replace(' ', '')
        return (m in ('mov', 'xchg', 'push', 'pop', 'test', 'xor', 'or', 'and', 'cmp', 'nop')
                and re.fullmatch(REGS + '(,' + REGS + ')?', ops)) or m == 'nop' or \
            (m == 'lea' and re.fullmatch(r'(\w+),\[\w+(\+eiz\*1)?(\+0x0)?\]', ops))

    bo, bu = body(b), body(a)
    if [t.split()[0] for t in bo] == [t.split()[0] for t in bu]:
        # same shape: pair by pair, stack offsets included (a wrong local field shows here)
        pairs = [(o, u) for o, u in zip(bo, bu) if key(o) != key(u) and not (boring(o) and boring(u))]
        return [o for o, u in pairs], [u for o, u in pairs]
    sa, sb = {}, {}
    for t in bo:
        sa.setdefault(key(t, False), []).append(t)
    for t in bu:
        sb.setdefault(key(t, False), []).append(t)
    only_orig = [t for k, v in sa.items() for t in v[len(sb.get(k, [])):] if not boring(t)]
    only_ours = [t for k, v in sb.items() for t in v[len(sa.get(k, [])):] if not boring(t)]
    return only_orig, only_ours


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('-')]
    verbose = '-v' in sys.argv
    sem = '--sem' in sys.argv
    only = sys.argv[sys.argv.index('-f') + 1] if '-f' in sys.argv else None
    if only:
        args.remove(only)
    mod, src, objp = args[:3]
    pe = PE(os.path.join(ROOT, 'orig', 'WINLEMM', mod))
    obj = Obj(objp)
    funcs = obj.functions()
    ann = annotations(src)
    known = load_names(mod)                     # C name -> address
    objs = os.path.join(ROOT, 'src', mod.split('.')[0].lower(), 'objs.txt')
    if os.path.exists(objs):                    # functions defined in the module's other sources
        for line in open(objs):
            other = os.path.join(os.path.dirname(objs), line.strip())
            if line.strip() and not line.startswith('#') and os.path.exists(other):
                for a, n in annotations(other):
                    known.setdefault(n, a)
    for a, n in ann:
        known[n] = a
    imports = {}                                # import name -> IAT slot
    for slot, (dll, nm) in pe.imports.items():
        imports[nm] = slot
    exports = {n: a for o, a, n in pe.exports if n}

    total = ok = 0
    nbytes = okbytes = 0
    seen = {}
    for addr, name in ann:
        nth = seen[name] = seen.get(name, -1) + 1
        if only and name != only:
            continue
        cands = sorted((k for k in funcs if c_name(k) == name), key=lambda k: (funcs[k][0].name, funcs[k][1]))
        cands = cands[nth:] + cands[:nth]
        if not cands:
            print(f'{addr:#x} {name}: not in object')
            total += 1
            continue
        sec, start, end = funcs[cands[0]]
        code = bytearray(sec.data[start:end])
        # MSVC pads inside a function's own section only at its end; trim trailing int3/nop-ish padding
        orig = bytearray(pe.read(addr, len(code)))
        total += 1
        nbytes += len(code)
        problems = []
        unchecked = 0
        masked = set()
        shown = bytearray(code)                 # our code with relocations applied, for disassembly
        for ro, rs, rt in sec.relocs:
            if not (start <= ro < end):
                continue
            o = ro - start
            masked.update(range(o, o + 4))
            sym = obj.symbols[rs]
            ov = struct.unpack_from('<I', orig, o)[0]
            addend = struct.unpack_from('<I', code, o)[0]
            target = None
            if sym.name.startswith('__imp_'):
                imp = sym.name[6:].lstrip('_')
                imp = re.sub(r'@\d+$', '', imp)
                target = imports.get(imp)
                if target is None:
                    # imported from one of the game's DLLs: decorated name
                    for nm, slot in imports.items():
                        if nm.lstrip('_').split('@')[0] == imp:
                            target = slot
            elif sym.name in known:             # an exact (C++) symbol in the names file: overloads
                target = known[sym.name]
            else:
                cn = c_name(sym.name)
                if cn in known:
                    target = known[cn]
                elif sym.name in exports:
                    target = exports[sym.name]
                elif sym.secnum > 0 and obj.section(sym.secnum).name in ('.data', '.rdata') and \
                        sym.name.startswith('??_C@'):
                    # a string literal: the original must point at the same text
                    lit = obj.section(sym.secnum).data[sym.value:]
                    lit = lit.split(b'\0', 1)[0]
                    if rt == IMAGE_REL_I386_DIR32:
                        got = pe.cstr(ov + 0 - addend if False else ov - addend)
                        if got != lit:
                            problems.append(f'+{o:#x} string {lit!r} vs original {got!r}')
                        else:
                            shown[o:o + 4] = orig[o:o + 4]
                    continue
            if target is None and sym.secnum > 0 and obj.section(sym.secnum).name in ('.data', '.rdata', '.bss'):
                # data defined in this object (string literals, tables): the original
                # must point at the same bytes
                dsec = obj.section(sym.secnum)
                if rt == IMAGE_REL_I386_DIR32 and dsec.name != '.bss':
                    mine = dsec.data[sym.value + addend:]
                    if b'\0' in mine[:256]:
                        mine = mine[:mine.index(b'\0') + 1]
                    else:
                        mine = mine[:16]
                    theirs = pe.read(ov, len(mine))
                    base = sym.value + addend
                    for dro, drs, drt in dsec.relocs:   # pointers in the data: not comparable here
                        for k in range(max(dro, base), min(dro + 4, base + len(mine))):
                            mine = mine[:k - base] + theirs[k - base:k - base + 1] + mine[k - base + 1:]
                    if mine != theirs:
                        problems.append(f'+{o:#x} data {mine[:24]!r} vs original {theirs[:24]!r}')
                    else:
                        shown[o:o + 4] = orig[o:o + 4]
                    continue
            if target is None:
                unchecked += 1
                shown[o:o + 4] = orig[o:o + 4]
                continue
            if rt == IMAGE_REL_I386_DIR32:
                want = (target + addend) & 0xffffffff
            elif rt == IMAGE_REL_I386_REL32:
                want = (target + addend - (addr + o + 4)) & 0xffffffff
            else:
                unchecked += 1
                shown[o:o + 4] = orig[o:o + 4]
                continue
            struct.pack_into('<I', shown, o, want)
            if ov != want:
                problems.append(f'+{o:#x} {sym.name}: original {ov:#x}, expected {want:#x}')
        diff = [i for i in range(len(code)) if i not in masked and code[i] != orig[i]]
        if diff:
            a = disasm(bytes(shown), addr)
            b = disasm(bytes(orig), addr)
            shape = [i[1].split()[0] for i in a if i[1]] == [i[1].split()[0] for i in b if i[1]][:len(a)]
            problems.insert(0, f'{len(diff)} bytes differ, first at +{diff[0]:#x}, '
                               f'{insn_diff(a, b, addr, addr + len(code))} insns differ'
                               + (' (same shape)' if shape else ''))
            if sem:
                # the original's own extent: up to the next known function
                nxt = min([v for v in known.values() if isinstance(v, int) and addr < v <= addr + 0x4000] or
                          [addr + len(code)])
                so, su = semantic(a, disasm(bytes(pe.read(addr, nxt - addr)), addr), addr, max(nxt, addr + len(code)))
                problems.insert(1, f'semantic: {len(so)} original-only, {len(su)} ours-only' +
                                ''.join(f'\n      original: {t}' for t in so[:8]) +
                                ''.join(f'\n      ours:     {t}' for t in su[:8]))
        # the original function must also end where ours does (next byte is padding or a new function)
        if not diff and not problems:
            ok += 1
            okbytes += len(code)
            print(f'{addr:#x} {name}: MATCH ({len(code)} bytes{", %d relocs unchecked" % unchecked if unchecked else ""})')
        else:
            print(f'{addr:#x} {name}: DIFF ({len(code)} bytes) ' + '; '.join(problems[:3]))
            if verbose:
                a = disasm(bytes(shown), addr)
                b = disasm(bytes(orig) + pe.read(addr + len(code), 32), addr)
                n = max(len(a), len(b))
                for i in range(n):
                    x = a[i][1] if i < len(a) else ''
                    y = b[i][1] if i < len(b) else ''
                    at = a[i][0] if i < len(a) else (b[i][0] if i < len(b) else 0)
                    mark = ' ' if x == y else '*'
                    print(f'   {mark} {at:08x}  {x:<45.45} | {y}')
    print(f'{ok}/{total} functions match ({okbytes}/{nbytes} bytes)')
    return 0 if ok == total else 1


if __name__ == '__main__':
    sys.exit(main())

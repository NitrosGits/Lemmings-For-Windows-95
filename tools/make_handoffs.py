#!/usr/bin/env python3
"""Writes handoffs/<Function>-handoff.md for every remaining mismatching function: current source,
the side-by-side diff (ours | original), and what HANDOFF.md says was already tried.
  make_handoffs.py            (list is FUNCS below; refresh it from a fresh verify run)"""
import os, re, subprocess, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FUNCS = [  # module, source, address, name, short (for HANDOFF.md grep)
 ('PROCESS.DLL','src/process/builder.cpp','0x10001690','Builder::Process','Builder'),
 ('PROCESS.DLL','src/process/basher.cpp','0x10001040','Basher::Process','Basher'),
 ('PROCESS.DLL','src/process/miner.cpp','0x10003c60','Miner::Process','Miner'),
 ('PROCESS.DLL','src/process/map.cpp','0x10004760','Map::AddObjectType','AddObjectType'),
 ('PROCESS.DLL','src/process/map.cpp','0x10004880','Map::AddTrapType','AddTrapType'),
 ('RENDER.DLL','','0x10001020','Renderer_Render','Renderer_Render'),
 ('RENDER.DLL','','0x10004af0','CopyChanged','CopyChanged'),
 ('LEMMINGS.EXE','','0x406490','ScrollTo','ScrollTo'),
 ('LEMMINGS.EXE','','0x406640','SetViewScale','SetViewScale'),
 ('LEMMINGS.EXE','','0x40b3e0','OpenLevelItem','OpenLevelItem'),
 ('LEMMINGS.EXE','','0x40b840','DrawPreview','DrawPreview'),
 ('LEMMINGS.EXE','','0x40bbe0','ShowLevelDetails','ShowLevelDetails'),
]
def find_src(addr):
    out = subprocess.run(['grep','-rl','/\\* '+addr,'src'],cwd=ROOT,capture_output=True,text=True).stdout.split()
    return out[0] if out else ''
hand = open(os.path.join(ROOT,'HANDOFF.md'),encoding='utf-8').read().splitlines()
for mod, src, addr, name, short in FUNCS:
    src = src if src and os.path.exists(os.path.join(ROOT,src)) and ('/* '+addr) in open(os.path.join(ROOT,src),encoding='latin1').read() else find_src(addr)
    text = open(os.path.join(ROOT,src),encoding='latin1').read()
    s = text.index('/* '+addr); e = text.index('\n}\n', s)+3
    code = text[s:e]
    r = subprocess.run(['zsh','tools/msvc/try.sh',mod,src,'-v','-f',name],cwd=ROOT,capture_output=True,text=True,timeout=900)
    lines = r.stdout.splitlines()
    head = next((l for l in lines if name in l and 'DIFF' in l or 'MATCH' in l and name in l), '(no result line)')
    lo = int(addr,16); size = int(re.search(r'\((\d+) bytes\)',head).group(1)) if '(' in head else 0
    side = []
    for l in lines:
        m = re.match(r'\s*\*?\s*([0-9a-f]{6,8})\s',l)
        if m and lo <= int(m.group(1),16) < lo+size: side.append(l)
    notes = [l[:700] for l in hand if re.search(r'\b'+re.escape(short)+r'\b',l)][-14:]
    md = f"""# {name} hand-off

> Generated {__import__('datetime').date.today()}. Project `{ROOT}` (not a git repo). Module **{mod}**, function at **{addr}**, source `{src}`.
> Status: `{head.strip()}`

## Goal
Make `{name}` compile with VC++ 2.2 (`/Ox`, via `tools/msvc/try.sh {mod} {src} -v -f {name}`) to bytes identical to the original, without breaking the other functions in `{os.path.basename(src)}` or the MinGW build (`make`).

## How to test
```
zsh tools/msvc/try.sh {mod} {src} -v -f {name}
python3 tools/msvc/realname.py {mod} {src} {addr} SPEC.py --jobs 3   # sweep source variants under the real file name
```
`realname.py`'s insn count can hide byte growth: always confirm with `try.sh`. See `docs/DECOMP.md` (padding externs, size-mode functions, block-scoped temporaries, inline-member expansion) and HANDOFF.md.

## Current source
```cpp
{code}```

## Side-by-side (ours | original; `*` marks differing rows)
```
{chr(10).join(side)}
```

## What HANDOFF.md already records (most recent last)
""" + "\n".join('- '+n for n in notes) + """

## Hints
- Differences are almost always a register/ordering choice by the compiler. Levers that have worked elsewhere: `extern int pad_xxxN;` symbol padding, declaration order and locals, block-scoped temporaries `{ T t = expr; v = t; }`, deriving a field from a just-stored field, `#pragma optimize("t", off)` plus a naked `_emit` padding stub for size-mode functions.
- Write new reusable scripts under `tools/`, record negatives in HANDOFF.md.
"""
    open(os.path.join(ROOT,'handoffs',name.replace('::','_')+'-handoff.md'),'w',encoding='utf-8').write(md)
    print(name, src, head[:60].strip(), len(side))

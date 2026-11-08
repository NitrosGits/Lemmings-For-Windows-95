#!/usr/bin/env python3
"""Model of C2.EXE's register colouring for one allocator iteration (VC++ 2.2, frame pointer omitted: K = 7).

usage: simplify_model.py NB_TXT        (NB_TXT = output of neigh_trace_fn.py for one function)
Rebuilds the interference graph of the LAST iteration in NB_TXT, re-runs simplify + colour and prints the colours, so
graph edits (add/remove edges, drop nodes, renumber ids) can be tried in Python before looking for a source change.
Import it and call run(edges, nodes) for experiments (see build/scratch/s40_claude/sim*.py for the CopyChanged ones).

Rules (decoded from sub_417690 / sub_417580, checked against the CopyChanged trace, exact order reproduced):
* worklist = 0x8xxx nodes bucket 31..0, then 0x4xxx nodes bucket 31..0 (bucket = id & 0x1f); nodes flagged 0x800
  (coalesced) are left out;
* repeat: if some node has degree < K remove the one with the HIGHEST such degree (tie: first in the worklist), else
  remove the one with the lowest metric (h=0: cost*100/degree, h=1: cost*100); removing decrements the neighbours' degree;
* colour in reverse removal order: first of eax, ecx, edx, esi, edi, ebx, ebp not used by a coloured neighbour
  (ids 0-7 are the machine registers with fixed colours, e.g. eax/ecx/edx around a call).
"""
import re
import sys

REGS = ['eax', 'ecx', 'edx', 'ebx', 'esp', 'ebp', 'esi', 'edi']
PREF = [0, 1, 2, 6, 7, 3, 5]
K = 7


def worder(ids):
    return sorted(ids, key=lambda i: (-(i >= 0x8000), -(i & 0x1f), -i))


def run(edges, nodes, fixed=None):
    """edges: iterable of (a, b) ids; nodes: virtual ids (>= 0x4000). Returns (colours {id: reg index or None},
    colouring order)."""
    fixed = dict(fixed or {r: r for r in range(8)})
    adj = {n: set() for n in list(nodes) + list(fixed)}
    for a, b in edges:
        adj.setdefault(a, set()).add(b)
        adj.setdefault(b, set()).add(a)
    deg = {n: len(adj[n]) for n in nodes}
    work = worder(nodes)
    removed = []
    while work:
        best = None
        for n in work:
            if best is None:
                best = n
            elif deg[best] < K:
                if deg[n] < K and deg[best] < deg[n]:
                    best = n
            elif deg[n] < K:
                best = n
        work.remove(best)
        removed.append(best)
        for m in adj[best]:
            if m in deg:
                deg[m] -= 1
    col = dict(fixed)
    for n in reversed(removed):
        used = {col[m] for m in adj[n] if m in col}
        col[n] = next((r for r in PREF if r not in used), None)
    return col, list(reversed(removed))


def from_neigh(path):
    """Graph of the last iteration in a neigh_trace_fn.py output (a new iteration starts when a node repeats)."""
    rows = []
    for line in open(path):
        m = re.match(r'([0-9a-f]{4}) \| (.*)', line.strip())
        if m:
            rows.append((int(m.group(1), 16), [int(t.split(':')[0], 16) for t in m.group(2).split()]))
    last, seen = [], set()
    for nid, nb in rows:
        if nid in seen:
            last, seen = [], set()
        seen.add(nid)
        last.append((nid, nb))
    edges = {tuple(sorted((a, b))) for a, nb in last for b in nb}
    nodes = sorted({x for e in edges for x in e if x >= 0x4000})
    return edges, nodes


if __name__ == '__main__':
    e, n = from_neigh(sys.argv[1])
    col, order = run(e, n)
    print(' '.join('%04x=%s' % (i, REGS[col[i]] if col[i] is not None else 'SPILL') for i in order))

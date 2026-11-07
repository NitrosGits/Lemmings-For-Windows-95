#!/usr/bin/env python3
"""Compare two harness lemming logs (LEMLOG.TXT format).

Each record line carries 0x80 raw bytes of a lemming. Dwords where both runs
hold a pointer (heap or module address, which differ between builds) are
ignored, and so is everything from byte UPTO on (default 0x30: the record's
tail holds fields the game never initialises, so heap leftovers). Any other
difference is reported.  cmplog.py A.txt B.txt [UPTO]
"""
import re
import struct
import sys


def is_ptr(v):
    return v >= 0x10000 and v != 0xffffffff and not (0xffff0000 <= v)


def main(a, b, upto=0x30):
    la = open(a).read().splitlines()
    lb = open(b).read().splitlines()
    bad = 0
    # the harness stops the game a moment after the last command, so one run
    # may have logged an update or two more: compare the updates both have
    if len(la) != len(lb):
        last = lambda ls: int(ls[-1].split()[0]) if ls else -1
        common = min(last(la), last(lb))
        la = [l for l in la if int(l.split()[0]) < common]
        lb = [l for l in lb if int(l.split()[0]) < common]
        print(f'comparing the first {common} updates')
    for n, (x, y) in enumerate(zip(la, lb)):
        if x == y:
            continue
        hx, _, rx = x.partition(' raw ')
        hy, _, ry = y.partition(' raw ')
        # "arg" is a stack address: which one depends on the code path that called
        hx = re.sub(r' arg \d+', ' arg *', hx)
        hy = re.sub(r' arg \d+', ' arg *', hy)
        if hx != hy:
            print(f'line {n + 1}: {hx} | {hy}')
            bad += 1
        else:
            bx, by = bytes.fromhex(rx), bytes.fromhex(ry)
            for o in range(0, min(len(bx), len(by), upto) - 3, 4):
                vx, vy = struct.unpack_from('<I', bx, o)[0], struct.unpack_from('<I', by, o)[0]
                if vx != vy and not (is_ptr(vx) and is_ptr(vy)):
                    print(f'line {n + 1} ({hx}): +{o:#x} {vx:#x} vs {vy:#x}')
                    bad += 1
                    break
        if bad > 20:
            break
    print('MATCH' if not bad else f'{bad}+ differences')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1], sys.argv[2], int(sys.argv[3], 0) if len(sys.argv) > 3 else 0x30))

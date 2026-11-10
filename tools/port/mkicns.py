#!/usr/bin/env python3
"""The macOS application icon from the game's own Windows icon:

    tools/port/mkicns.py src/lemmings/res/lemmings.ico build/app/Lemmings.icns [preview.png]

The Windows icon is a 32 x 32, 16-colour square tile (a yellow button with the
Lemming's head). A Mac's icon is a rounded square on a 1024-pixel canvas, so
the head is taken off its tile (the yellow round it becomes clear) and put,
its pixels enlarged a whole number of times, on a rounded square of the
tile's yellow. Written out at every size an .icns holds. No modules beyond
Python's own; the optional third argument also writes the largest size as a
PNG to look at.
"""
import struct
import sys
import zlib

CANVAS = 1024           # Apple's grid: the shape is 824 wide on a 1024 canvas
SHAPE = 824
EXPONENT = 5.0          # a superellipse: near enough the system's rounded square
TOP = (255, 238, 60)    # the tile's yellow, a little lighter above than below
BOTTOM = (250, 200, 0)
ART = 0.74              # the head's share of the shape


def read_ico(path):
    """The first image of an .ico: rows of (r, g, b, a), top first."""
    d = open(path, 'rb').read()
    _, _, count = struct.unpack_from('<HHH', d, 0)
    if not count:
        raise SystemExit('no image in ' + path)
    w, h, _, _, _, _, _, off = struct.unpack_from('<BBBBHHII', d, 6)
    w, h = w or 256, h or 256
    size, _, _, _, bpp, comp = struct.unpack_from('<IiiHHI', d, off)
    if comp or bpp not in (4, 8):
        raise SystemExit('only uncompressed 16- and 256-colour icons')
    ncol = 1 << bpp
    pal = [struct.unpack_from('<BBBB', d, off + size + i * 4) for i in range(ncol)]
    xor = off + size + ncol * 4
    pitch = ((w * bpp + 31) // 32) * 4
    mask = xor + pitch * h
    mpitch = ((w + 31) // 32) * 4
    rows = []
    for y in range(h):
        src = xor + (h - 1 - y) * pitch
        msrc = mask + (h - 1 - y) * mpitch
        row = []
        for x in range(w):
            if bpp == 4:
                i = d[src + x // 2] >> (0 if x & 1 else 4) & 15
            else:
                i = d[src + x]
            clear = d[msrc + x // 8] >> (7 - x % 8) & 1
            b, g, r, _ = pal[i]
            row.append((r, g, b, 0 if clear else 255))
        rows.append(row)
    return rows


def cut_out(rows):
    """The picture without its tile: the border's bevel off, and the tile's
    colour clear wherever it reaches the edge."""
    rows = [r[2:-2] for r in rows[2:-2]]
    h, w = len(rows), len(rows[0])
    tile = rows[h - 1][0][:3]
    todo = [(x, y) for x in range(w) for y in (0, h - 1)] + [(x, y) for y in range(h) for x in (0, w - 1)]
    while todo:
        x, y = todo.pop()
        if 0 <= x < w and 0 <= y < h and rows[y][x][3] and rows[y][x][:3] == tile:
            rows[y][x] = (0, 0, 0, 0)
            todo += [(x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)]
    ys = [y for y in range(h) if any(p[3] for p in rows[y])]
    xs = [x for x in range(w) if any(rows[y][x][3] for y in range(h))]
    return [r[xs[0]:xs[-1] + 1] for r in rows[ys[0]:ys[-1] + 1]]


def inside(x, y):
    """How much of canvas pixel x, y the rounded square covers, 0..1."""
    c, half = CANVAS / 2, SHAPE / 2

    def f(px, py):
        return (abs(px - c) / half) ** EXPONENT + (abs(py - c) / half) ** EXPONENT

    v = f(x + 0.5, y + 0.5)
    if v < 0.97:
        return 1.0
    if v > 1.03:
        return 0.0
    n = sum(f(x + (i + 0.5) / 4, y + (j + 0.5) / 4) <= 1 for i in range(4) for j in range(4))
    return n / 16


def master(art):
    """The 1024-pixel icon: flat bytes, r g b a, alpha not premultiplied."""
    ah, aw = len(art), len(art[0])
    k = int(SHAPE * ART) // max(aw, ah)
    ox, oy = (CANVAS - aw * k) // 2, (CANVAS - ah * k) // 2
    top = (CANVAS - SHAPE) // 2
    out = bytearray(CANVAS * CANVAS * 4)
    for y in range(CANVAS):
        t = min(1.0, max(0.0, (y - top) / SHAPE))
        plate = tuple(round(TOP[i] + (BOTTOM[i] - TOP[i]) * t) for i in range(3))
        ay = (y - oy) // k if oy <= y < oy + ah * k else -1
        for x in range(CANVAS):
            a = inside(x, y)
            if not a:
                continue
            p = plate
            if ay >= 0 and ox <= x < ox + aw * k:
                q = art[ay][(x - ox) // k]
                if q[3]:
                    p = q
            o = (y * CANVAS + x) * 4
            out[o], out[o + 1], out[o + 2], out[o + 3] = p[0], p[1], p[2], round(a * 255)
    return out


def halve(px, n):
    """An n-pixel image at half the size: each pixel the mean of four (weighted by alpha)."""
    m = n // 2
    out = bytearray(m * m * 4)
    for y in range(m):
        for x in range(m):
            r = g = b = a = 0
            for dy in (0, 1):
                for dx in (0, 1):
                    o = ((y * 2 + dy) * n + x * 2 + dx) * 4
                    al = px[o + 3]
                    r += px[o] * al
                    g += px[o + 1] * al
                    b += px[o + 2] * al
                    a += al
            o = (y * m + x) * 4
            if a:
                out[o], out[o + 1], out[o + 2], out[o + 3] = round(r / a), round(g / a), round(b / a), round(a / 4)
    return out


def png(px, n):
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))

    raw = b''.join(b'\0' + bytes(px[y * n * 4:(y + 1) * n * 4]) for y in range(n))
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', n, n, 8, 6, 0, 0, 0)) +
            chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))


def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    sizes = {CANVAS: master(cut_out(read_ico(sys.argv[1])))}
    n = CANVAS
    while n > 16:
        sizes[n // 2] = halve(sizes[n], n)
        n //= 2
    pngs = {n: png(px, n) for n, px in sizes.items()}
    # each type's pixel size (icXX at 2x holds twice the points)
    kinds = [(b'icp4', 16), (b'icp5', 32), (b'icp6', 64), (b'ic07', 128), (b'ic08', 256), (b'ic09', 512),
             (b'ic10', 1024), (b'ic11', 32), (b'ic12', 64), (b'ic13', 256), (b'ic14', 512)]
    body = b''.join(kind + struct.pack('>I', len(pngs[n]) + 8) + pngs[n] for kind, n in kinds)
    with open(sys.argv[2], 'wb') as f:
        f.write(b'icns' + struct.pack('>I', len(body) + 8) + body)
    if len(sys.argv) > 3:
        with open(sys.argv[3], 'wb') as f:
            f.write(pngs[CANVAS])


if __name__ == '__main__':
    main()

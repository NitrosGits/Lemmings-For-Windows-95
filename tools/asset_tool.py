#!/usr/bin/env python3
"""Lemmings for Windows 95 - Asset Extraction, Inspection & Packing Tool.

Zero-dependency (pure Python stdlib) utility for reading, viewing, exporting,
editing, and repacking all proprietary binary assets of Lemmings 95:
  - .SPR : Run-Length Encoded (RLE) sprite sheets & animation frames
  - .PAL : 256-color frontend and 64-color style color palettes
  - .DAT : Interactive object definitions (triggers, traps, doors, sounds)
  - .LVL : Big-endian level descriptors, terrain piece blits & steel zones
  - .BMP : 16-color & 256-color level selection thumbnails & UI graphics

Used to create 100% open, non-copyrighted replacement assets for the game.
"""

import argparse
import binascii
import glob
import json
import math
import os
import struct
import sys
import zlib


# ============================================================================
# Pure-Python Standalone PNG Encoder & Decoder (No Pillow Required)
# ============================================================================

class PNG:
    """Read and write PNG files using Python's built-in zlib and struct."""

    MAGIC = b'\x89PNG\r\n\x1a\n'

    @staticmethod
    def _chunk(tag: bytes, data: bytes) -> bytes:
        crc = binascii.crc32(tag + data) & 0xffffffff
        return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', crc)

    @classmethod
    def write_indexed(cls, width: int, height: int, palette: list,
                      indices: bytes, trans_idx: int = 0) -> bytes:
        """Write an 8-bit indexed PNG.

        palette is a list of (r, g, b) tuples (0..255).
        indices is bytes of length width * height.
        trans_idx is the palette index to treat as completely transparent (or None).
        """
        ihdr = struct.pack('>IIBBBBB', width, height, 8, 3, 0, 0, 0)

        # PLTE chunk: 3 bytes per color
        plte = bytearray()
        for r, g, b in palette:
            plte.extend((int(r) & 0xff, int(g) & 0xff, int(b) & 0xff))

        # Pad palette up to at least trans_idx + 1 if needed
        min_len = (trans_idx + 1) if trans_idx is not None else 1
        while len(plte) < min_len * 3:
            plte.extend((0, 0, 0))

        # Scanlines: filter byte 0 (None) prefixed to each row
        raw = bytearray()
        for y in range(height):
            raw.append(0)
            raw.extend(indices[y * width : (y + 1) * width])

        idat = zlib.compress(bytes(raw), 9)

        out = bytearray(cls.MAGIC)
        out.extend(cls._chunk(b'IHDR', ihdr))
        out.extend(cls._chunk(b'PLTE', bytes(plte)))

        if trans_idx is not None:
            # tRNS chunk for indexed: alpha bytes up to trans_idx
            trns = bytearray(trans_idx + 1)
            for i in range(trans_idx):
                trns[i] = 255
            trns[trans_idx] = 0
            out.extend(cls._chunk(b'tRNS', bytes(trns)))

        out.extend(cls._chunk(b'IDAT', idat))
        out.extend(cls._chunk(b'IEND', b''))
        return bytes(out)

    @classmethod
    def write_rgba(cls, width: int, height: int, rgba: bytes) -> bytes:
        """Write a 32-bit RGBA PNG."""
        ihdr = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)
        raw = bytearray()
        stride = width * 4
        for y in range(height):
            raw.append(0)
            raw.extend(rgba[y * stride : (y + 1) * stride])

        idat = zlib.compress(bytes(raw), 9)
        out = bytearray(cls.MAGIC)
        out.extend(cls._chunk(b'IHDR', ihdr))
        out.extend(cls._chunk(b'IDAT', idat))
        out.extend(cls._chunk(b'IEND', b''))
        return bytes(out)

    @classmethod
    def read(cls, data: bytes):
        """Read a PNG file, returning (width, height, mode, pixels, palette, trans_idx).

        mode: 'indexed' (pixels=bytearray of indices), 'rgba' (pixels=bytearray of RGBA),
              or 'rgb' (pixels=bytearray of RGB).
        """
        if not data.startswith(cls.MAGIC):
            raise ValueError("Not a valid PNG file")

        p = 8
        width = height = bit_depth = color_type = 0
        palette = []
        trns = None
        idat = bytearray()

        while p < len(data):
            length, tag = struct.unpack_from('>I4s', data, p)
            chunk_data = data[p + 8 : p + 8 + length]
            p += 12 + length

            if tag == b'IHDR':
                width, height, bit_depth, color_type = struct.unpack('>IIBB', chunk_data[:10])
            elif tag == b'PLTE':
                palette = [tuple(chunk_data[i:i+3]) for i in range(0, len(chunk_data), 3)]
            elif tag == b'tRNS':
                trns = chunk_data
            elif tag == b'IDAT':
                idat.extend(chunk_data)
            elif tag == b'IEND':
                break

        decomp = zlib.decompress(bytes(idat))

        # Helper to unfilter scanlines
        def unfilter(bpp):
            stride = 1 + width * bpp
            rows = []
            prev_row = bytearray(width * bpp)
            for y in range(height):
                filt = decomp[y * stride]
                raw = bytearray(decomp[y * stride + 1 : (y + 1) * stride])
                if filt == 1:  # Sub
                    for x in range(bpp, width * bpp):
                        raw[x] = (raw[x] + raw[x - bpp]) & 0xff
                elif filt == 2:  # Up
                    for x in range(width * bpp):
                        raw[x] = (raw[x] + prev_row[x]) & 0xff
                elif filt == 3:  # Average
                    for x in range(width * bpp):
                        left = raw[x - bpp] if x >= bpp else 0
                        raw[x] = (raw[x] + ((left + prev_row[x]) >> 1)) & 0xff
                elif filt == 4:  # Paeth
                    for x in range(width * bpp):
                        a = raw[x - bpp] if x >= bpp else 0
                        b = prev_row[x]
                        c = prev_row[x - bpp] if x >= bpp else 0
                        pa = abs(b - c)
                        pb = abs(a - c)
                        pc = abs(a + b - 2 * c)
                        pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                        raw[x] = (raw[x] + pr) & 0xff
                rows.append(raw)
                prev_row = raw
            return b''.join(rows)

        trans_idx = None
        if color_type == 3:  # Indexed
            pixels = unfilter(1)
            if trns:
                # Find zero alpha in tRNS
                for idx, a in enumerate(trns):
                    if a == 0:
                        trans_idx = idx
                        break
            return width, height, 'indexed', bytearray(pixels), palette, trans_idx
        elif color_type == 6:  # RGBA
            pixels = unfilter(4)
            return width, height, 'rgba', bytearray(pixels), None, None
        elif color_type == 2:  # RGB
            pixels = unfilter(3)
            return width, height, 'rgb', bytearray(pixels), None, None
        else:
            raise ValueError(f"Unsupported PNG color type {color_type} / bit depth {bit_depth}")


# ============================================================================
# Pure-Python Standalone BMP Reader & Writer
# ============================================================================

class BMP:
    """Read and write standard Windows DIB/BMP images."""

    @staticmethod
    def read(data: bytes):
        if not data.startswith(b'BM'):
            raise ValueError("Not a valid BMP file")
        off_bits = struct.unpack_from('<I', data, 10)[0]
        bi_size, w, h, planes, bpp, comp = struct.unpack_from('<IiiHHI', data, 14)
        top_down = False
        if h < 0:
            h = -h
            top_down = True

        palette = []
        if bpp <= 8:
            num_colors = 1 << bpp
            pal_bytes = data[14 + bi_size : 14 + bi_size + num_colors * 4]
            for i in range(num_colors):
                b, g, r, _ = pal_bytes[i*4 : (i+1)*4]
                palette.append((r, g, b))

        # Row stride in BMP is padded to multiple of 4 bytes
        row_stride = ((w * bpp + 31) // 32) * 4
        pixel_data = bytearray(w * h)

        if bpp == 8:
            for y in range(h):
                src_y = y if top_down else (h - 1 - y)
                row = data[off_bits + src_y * row_stride : off_bits + src_y * row_stride + w]
                pixel_data[y * w : (y + 1) * w] = row
            return w, h, 'indexed', pixel_data, palette
        elif bpp == 4:
            for y in range(h):
                src_y = y if top_down else (h - 1 - y)
                row_raw = data[off_bits + src_y * row_stride : off_bits + (src_y + 1) * row_stride]
                row = bytearray(w)
                for x in range(w):
                    byte_val = row_raw[x // 2]
                    row[x] = (byte_val >> 4) if (x % 2 == 0) else (byte_val & 0x0f)
                pixel_data[y * w : (y + 1) * w] = row
            return w, h, 'indexed', pixel_data, palette
        elif bpp == 24:
            rgb_data = bytearray(w * h * 3)
            for y in range(h):
                src_y = y if top_down else (h - 1 - y)
                row = data[off_bits + src_y * row_stride : off_bits + src_y * row_stride + w * 3]
                # BGR to RGB
                for x in range(w):
                    b = row[x * 3]
                    g = row[x * 3 + 1]
                    r = row[x * 3 + 2]
                    dst_idx = (y * w + x) * 3
                    rgb_data[dst_idx : dst_idx + 3] = bytes([r, g, b])
            return w, h, 'rgb', rgb_data, None
        else:
            raise ValueError(f"Unsupported BMP bit depth: {bpp}")


# ============================================================================
# Color Palette (.PAL)
# ============================================================================

class Palette:
    """Represents a game color palette (.PAL)."""

    MAGIC = 1346456608  # 0x50414C20 (' PAL' in little-endian)

    def __init__(self, colors=None, count: int = None):
        self.colors = list(colors) if colors else [(0, 0, 0)] * 256
        self.count = count if count is not None else len(self.colors)

    @classmethod
    def load(cls, data: bytes):
        if len(data) < 6:
            raise ValueError("Palette data too short")
        magic, count = struct.unpack_from('<IH', data, 0)
        colors = []
        for i in range(count):
            r, g, b, _ = struct.unpack_from('BBBB', data, 6 + i * 4)
            colors.append((r, g, b))
        # Pad to 256 for internal canvas operations
        while len(colors) < 256:
            colors.append((0, 0, 0))
        return cls(colors, count=count)

    def to_bytes(self, count: int = None) -> bytes:
        if count is None:
            count = self.count
        out = bytearray(struct.pack('<IH', self.MAGIC, count))
        for i in range(count):
            r, g, b = self.colors[i] if i < len(self.colors) else (0, 0, 0)
            out.extend((r & 0xff, g & 0xff, b & 0xff, 0))
        return bytes(out)

    def to_json(self) -> str:
        arr = []
        for i in range(self.count):
            r, g, b = self.colors[i] if i < len(self.colors) else (0, 0, 0)
            arr.append({
                "index": i,
                "r": r,
                "g": g,
                "b": b,
                "hex": f"#{r:02x}{g:02x}{b:02x}"
            })
        return json.dumps({
            "count": self.count,
            "colors": arr
        }, indent=2)

    @classmethod
    def from_json(cls, json_str: str):
        parsed = json.loads(json_str)
        if isinstance(parsed, list):
            count = len(parsed)
            arr = parsed
        else:
            count = parsed.get("count", len(parsed.get("colors", [])))
            arr = parsed.get("colors", [])
        colors = [(0, 0, 0)] * 256
        for item in arr:
            idx = item.get("index", 0)
            if "hex" in item:
                h = item["hex"].lstrip("#")
                r = int(h[0:2], 16)
                g = int(h[2:4], 16)
                b = int(h[4:6], 16)
            else:
                r = item.get("r", 0)
                g = item.get("g", 0)
                b = item.get("b", 0)
            if 0 <= idx < len(colors):
                colors[idx] = (r, g, b)
        return cls(colors, count=count)

    def render_swatch(self, cell_size: int = 16, cols: int = 16) -> bytes:
        """Render a visual color swatch sheet PNG of the palette."""
        rows = (len(self.colors) + cols - 1) // cols
        width = cols * cell_size
        height = rows * cell_size
        canvas = bytearray(width * height)

        for i, (r, g, b) in enumerate(self.colors):
            cx = (i % cols) * cell_size
            cy = (i // cols) * cell_size
            for y in range(cell_size):
                for x in range(cell_size):
                    # 1px border around each swatch
                    if x == 0 or y == 0 or x == cell_size - 1 or y == cell_size - 1:
                        canvas[(cy + y) * width + (cx + x)] = 0
                    else:
                        canvas[(cy + y) * width + (cx + x)] = i

        return PNG.write_indexed(width, height, self.colors, bytes(canvas), trans_idx=None)


# ============================================================================
# Sprite & Frame Handling (.SPR)
# ============================================================================

class SpriteFrame:
    """A single sprite animation frame."""

    def __init__(self, x: int = 0, y: int = 0, width: int = 0, height: int = 0,
                 orig_w: int = 0, orig_h: int = 0, pixels: bytearray = None):
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.orig_w = orig_w or width
        self.orig_h = orig_h or height
        self.pixels = pixels or bytearray(width * height)

    def encode_rle(self) -> bytes:
        """Compress pixels into the exact WinLemm / DMA Design RLE format."""
        out = bytearray(struct.pack('<HHHHHH', self.x, self.y, self.width, self.height,
                                    self.orig_w, self.orig_h))
        for row_idx in range(self.height):
            row = self.pixels[row_idx * self.width : (row_idx + 1) * self.width]
            # Trim trailing transparent (0) pixels before 0x80 end-of-row
            last = len(row) - 1
            while last >= 0 and row[last] == 0:
                last -= 1

            i = 0
            w = last + 1
            while i < w:
                if row[i] == 0:
                    start = i
                    while i < w and row[i] == 0 and (i - start) < 127:
                        i += 1
                    out.append(i - start)
                else:
                    start = i
                    while i < w and row[i] != 0 and (i - start) < 127:
                        i += 1
                    n = i - start
                    out.append(0x80 | n)
                    out.extend(row[start:i])
            out.append(0x80)  # End of row
        return bytes(out)

    @classmethod
    def decode_rle(cls, data: bytes, offset: int):
        """Unpack a single sprite frame from binary .SPR data at offset."""
        x, y, w, h, orig_w, orig_h = struct.unpack_from('<HHHHHH', data, offset)
        p = offset + 12
        pixels = bytearray(w * h)
        for row_idx in range(h):
            col = 0
            while True:
                c = data[p]
                p += 1
                if c == 0x80:
                    break
                elif c > 0x80:
                    n = c & 0x7f
                    for i in range(n):
                        pixels[row_idx * w + col + i] = data[p + i]
                    p += n
                    col += n
                else:
                    col += c
        return cls(x, y, w, h, orig_w, orig_h, pixels)


class SpriteFile:
    """A multi-frame sprite resource (.SPR)."""

    MAGIC = b'SRLE'

    def __init__(self, frames=None):
        self.frames = list(frames) if frames else []

    @classmethod
    def load(cls, data: bytes):
        if data.startswith(b'BMPSpritePos'):
            raise ValueError("File is legacy BMPSpritePos table, not SRLE sprite file")
        if not data.startswith(cls.MAGIC):
            raise ValueError(f"Invalid sprite magic: expected SRLE, got {data[:4]!r}")

        count = struct.unpack_from('<H', data, 4)[0]
        offsets = struct.unpack_from(f'<{count}I', data, 6)
        frames = []
        for o in offsets:
            frames.append(SpriteFrame.decode_rle(data, o))
        return cls(frames)

    def to_bytes(self) -> bytes:
        """Serialize all frames to binary .SPR with SRLE header."""
        count = len(self.frames)
        encoded_frames = [f.encode_rle() for f in self.frames]

        header = bytearray(self.MAGIC + struct.pack('<H', count))
        cur_offset = 6 + 4 * count
        offsets = []
        for fbytes in encoded_frames:
            offsets.append(cur_offset)
            cur_offset += len(fbytes)

        header.extend(struct.pack(f'<{count}I', *offsets))
        return bytes(header + b''.join(encoded_frames))

    def extract(self, out_dir: str, palette: Palette = None, make_sheet: bool = True):
        """Extract all frames to PNG files and metadata.json in out_dir."""
        os.makedirs(out_dir, exist_ok=True)
        pal_colors = palette.colors if palette else [(i, i, i) for i in range(256)]

        meta = {
            "magic": "SRLE",
            "frame_count": len(self.frames),
            "frames": []
        }

        for idx, f in enumerate(self.frames):
            fname = f"frame_{idx:03d}.png"
            fpath = os.path.join(out_dir, fname)
            png_bytes = PNG.write_indexed(f.width, f.height, pal_colors, bytes(f.pixels), trans_idx=0)
            with open(fpath, 'wb') as fp:
                fp.write(png_bytes)

            meta["frames"].append({
                "index": idx,
                "file": fname,
                "x": f.x,
                "y": f.y,
                "width": f.width,
                "height": f.height,
                "canvas_width": f.orig_w,
                "canvas_height": f.orig_h
            })

        meta_path = os.path.join(out_dir, "metadata.json")
        with open(meta_path, 'w') as fp:
            json.dump(meta, fp, indent=2)

        # Generate a composite sprite sheet image
        if make_sheet and self.frames:
            cols = min(8, len(self.frames))
            rows = (len(self.frames) + cols - 1) // cols
            max_w = max(f.width for f in self.frames) if self.frames else 1
            max_h = max(f.height for f in self.frames) if self.frames else 1
            pad = 2
            cell_w = max_w + pad * 2
            cell_h = max_h + pad * 2
            sheet_w = cols * cell_w
            sheet_h = rows * cell_h
            sheet = bytearray(sheet_w * sheet_h)

            for idx, f in enumerate(self.frames):
                cx = (idx % cols) * cell_w + pad
                cy = (idx // cols) * cell_h + pad
                for y in range(f.height):
                    for x in range(f.width):
                        sheet[(cy + y) * sheet_w + (cx + x)] = f.pixels[y * f.width + x]

            sheet_png = PNG.write_indexed(sheet_w, sheet_h, pal_colors, bytes(sheet), trans_idx=0)
            with open(os.path.join(out_dir, "spritesheet.png"), 'wb') as fp:
                fp.write(sheet_png)

    @classmethod
    def from_dir(cls, in_dir: str, palette: Palette = None):
        """Construct a SpriteFile from metadata.json and frame PNG/BMP files."""
        meta_path = os.path.join(in_dir, "metadata.json")
        if not os.path.exists(meta_path):
            raise FileNotFoundError(f"Missing {meta_path}")

        with open(meta_path, 'r') as fp:
            meta = json.load(fp)

        pal_colors = palette.colors if palette else None

        frames = []
        for item in meta["frames"]:
            fpath = os.path.join(in_dir, item["file"])
            with open(fpath, 'rb') as fp:
                img_data = fp.read()

            if img_data.startswith(PNG.MAGIC):
                w, h, mode, pixels, png_pal, trans_idx = PNG.read(img_data)
            elif img_data.startswith(b'BM'):
                w, h, mode, pixels, png_pal = BMP.read(img_data)
                trans_idx = 0
            else:
                raise ValueError(f"Unsupported image format in {fpath}")

            # Normalize pixels to 8-bit palette indices
            if mode == 'indexed':
                idx_pixels = pixels
                if trans_idx is not None and trans_idx != 0:
                    # remap trans_idx to 0
                    remap = bytearray(len(idx_pixels))
                    for i, px in enumerate(idx_pixels):
                        if px == trans_idx:
                            remap[i] = 0
                        elif px == 0:
                            remap[i] = trans_idx
                        else:
                            remap[i] = px
                    idx_pixels = remap
            elif mode == 'rgba':
                idx_pixels = bytearray(w * h)
                for i in range(w * h):
                    a = pixels[i * 4 + 3]
                    if a < 128:
                        idx_pixels[i] = 0
                    else:
                        r, g, b = pixels[i * 4 : i * 4 + 3]
                        # Find closest palette color (ignoring index 0 transparent)
                        if pal_colors:
                            best_idx = 1
                            best_dist = 9999999
                            for ci in range(1, len(pal_colors)):
                                cr, cg, cb = pal_colors[ci]
                                dist = (r - cr)**2 + (g - cg)**2 + (b - cb)**2
                                if dist < best_dist:
                                    best_dist = dist
                                    best_idx = ci
                            idx_pixels[i] = best_idx
                        else:
                            idx_pixels[i] = max(1, (r + g + b) // 3)
            elif mode == 'rgb':
                idx_pixels = bytearray(w * h)
                for i in range(w * h):
                    r, g, b = pixels[i * 3 : i * 3 + 3]
                    if r == 0 and g == 0 and b == 0:
                        idx_pixels[i] = 0
                    elif pal_colors:
                        best_idx = 1
                        best_dist = 9999999
                        for ci in range(1, len(pal_colors)):
                            cr, cg, cb = pal_colors[ci]
                            dist = (r - cr)**2 + (g - cg)**2 + (b - cb)**2
                            if dist < best_dist:
                                best_dist = dist
                                best_idx = ci
                        idx_pixels[i] = best_idx
                    else:
                        idx_pixels[i] = max(1, (r + g + b) // 3)

            frames.append(SpriteFrame(
                x=item.get("x", 0),
                y=item.get("y", 0),
                width=w,
                height=h,
                orig_w=item.get("canvas_width", w),
                orig_h=item.get("canvas_height", h),
                pixels=idx_pixels
            ))

        return cls(frames)


# ============================================================================
# Interactive Object Definitions (GROUND*.DAT)
# ============================================================================

class GroundData:
    """Interactive objects defined in STYLES/GROUND*O.DAT."""

    STYLE_NAMES = [
        "DIRT", "FIRE", "MARBLE", "PILLAR", "CRYSTAL",
        "BRICK", "ROCK", "SNOW", "BUBBLE"
    ]

    STYLE_GROUND_FILES = [
        "Ground0o.dat", "Ground1o.dat", "Ground2o.dat", "Ground3o.dat",
        "Ground4o.dat", "brick.dat", "rock.dat", "snow.dat",
        "bubbles.dat"
    ]
    STYLE_TERRAIN_SPRITES = [
        "level0s.spr", "level1s.spr", "level2s.spr", "level3s.spr",
        "level4s.spr", "bricks.spr", "rocks.spr", "snows.spr",
        "bubbles.spr"
    ]
    STYLE_TERRAIN_SPRITES_HI = [
        "level0b.spr", "level1b.spr", "level2b.spr", "level3b.spr",
        "level4b.spr", "brickb.spr", "rockb.spr", "snowb.spr",
        "bubbleb.spr"
    ]
    STYLE_OBJ_SPRITES = [
        "level0os.spr", "level1os.spr", "level2os.spr", "level3os.spr",
        "level4os.spr", "brickos.spr", "rockos.spr", "snowos.spr",
        "bubbleos.spr"
    ]
    STYLE_OBJ_SPRITES_HI = [
        "level0ob.spr", "level1ob.spr", "level2ob.spr", "level3ob.spr",
        "level4ob.spr", "brickob.spr", "rockob.spr", "snowob.spr",
        "bubbleob.spr"
    ]
    STYLE_PALETTES = [
        "level0.pal", "level1.pal", "level2.pal", "level3.pal",
        "level4.pal", "brick.pal", "rock.pal", "snow.pal",
        "bubble.pal"
    ]

    @staticmethod
    def _find_file(dir_path: str, filename: str) -> str:
        """Find a file in dir_path with case-insensitive fallback."""
        p = os.path.join(dir_path, filename)
        if os.path.exists(p):
            return p
        lower = filename.lower()
        if os.path.exists(dir_path):
            try:
                for f in os.listdir(dir_path):
                    if f.lower() == lower:
                        return os.path.join(dir_path, f)
            except OSError:
                pass
        return p

    def __init__(self, objects=None, raw_tail=b''):
        self.objects = list(objects) if objects else []
        self.raw_tail = raw_tail

    @classmethod
    def load(cls, data: bytes):
        objects = []
        # First 16 entries * 28 bytes = 448 bytes
        for i in range(16):
            chunk = data[i * 28 : (i + 1) * 28]
            if len(chunk) < 28:
                break
            (flags, flags2, start_frame, end_frame, width, height) = struct.unpack_from('<BBBBBB', chunk, 0)
            other = chunk[6:14]
            (trig_left, trig_top) = struct.unpack_from('<HH', chunk, 14)
            trig_w, trig_h, trig_effect = struct.unpack_from('<BBB', chunk, 18)
            other2 = chunk[21:27]
            sound = chunk[27]

            objects.append({
                "type_id": i,
                "flags": flags,
                "flags2": flags2,
                "start_frame": start_frame,
                "end_frame": end_frame,
                "width": width,
                "height": height,
                "trigger_left": trig_left,
                "trigger_top": trig_top,
                "trigger_width": trig_w,
                "trigger_height": trig_h,
                "trigger_effect": trig_effect,
                "sound": sound,
                "raw_other": binascii.hexlify(other).decode('ascii'),
                "raw_other2": binascii.hexlify(other2).decode('ascii')
            })
        return cls(objects, data[448:])

    def to_bytes(self) -> bytes:
        out = bytearray()
        for i in range(16):
            if i < len(self.objects):
                obj = self.objects[i]
                other = binascii.unhexlify(obj.get("raw_other", "00" * 8))
                other2 = binascii.unhexlify(obj.get("raw_other2", "00" * 6))
                chunk = bytearray(28)
                struct.pack_into('<BBBBBB', chunk, 0,
                                 obj.get("flags", 0),
                                 obj.get("flags2", 0),
                                 obj.get("start_frame", 0),
                                 obj.get("end_frame", 0),
                                 obj.get("width", 0),
                                 obj.get("height", 0))
                chunk[6:14] = other
                struct.pack_into('<HHBBB', chunk, 14,
                                 obj.get("trigger_left", 0),
                                 obj.get("trigger_top", 0),
                                 obj.get("trigger_width", 0),
                                 obj.get("trigger_height", 0),
                                 obj.get("trigger_effect", 0))
                chunk[21:27] = other2
                chunk[27] = obj.get("sound", 0)
                out.extend(chunk)
            else:
                out.extend(b'\x00' * 28)
        out.extend(self.raw_tail)
        return bytes(out)

    def to_json(self) -> str:
        data = {
            "objects": self.objects,
            "raw_tail_hex": binascii.hexlify(self.raw_tail).decode('ascii')
        }
        return json.dumps(data, indent=2)

    @classmethod
    def from_json(cls, json_str: str):
        parsed = json.loads(json_str)
        if isinstance(parsed, list):
            return cls(parsed, b'')
        objects = parsed.get("objects", [])
        tail = binascii.unhexlify(parsed.get("raw_tail_hex", "")) if "raw_tail_hex" in parsed else b''
        return cls(objects, tail)


# ============================================================================
# Level Descriptor (.LVL)
# ============================================================================

class LevelFile:
    """A big-endian 2048-byte level descriptor (.LVL)."""

    STYLE_NAMES = [
        "DIRT", "FIRE", "MARBLE", "PILLAR", "CRYSTAL",
        "BRICK", "ROCK", "SNOW", "BUBBLE"
    ]

    STYLE_GROUND_FILES = [
        "Ground0o.dat", "Ground1o.dat", "Ground2o.dat", "Ground3o.dat",
        "Ground4o.dat", "brick.dat", "rock.dat", "snow.dat",
        "bubbles.dat"
    ]
    STYLE_TERRAIN_SPRITES = [
        "level0s.spr", "level1s.spr", "level2s.spr", "level3s.spr",
        "level4s.spr", "bricks.spr", "rocks.spr", "snows.spr",
        "bubbles.spr"
    ]
    STYLE_TERRAIN_SPRITES_HI = [
        "level0b.spr", "level1b.spr", "level2b.spr", "level3b.spr",
        "level4b.spr", "brickb.spr", "rockb.spr", "snowb.spr",
        "bubbleb.spr"
    ]
    STYLE_OBJ_SPRITES = [
        "level0os.spr", "level1os.spr", "level2os.spr", "level3os.spr",
        "level4os.spr", "brickos.spr", "rockos.spr", "snowos.spr",
        "bubbleos.spr"
    ]
    STYLE_OBJ_SPRITES_HI = [
        "level0ob.spr", "level1ob.spr", "level2ob.spr", "level3ob.spr",
        "level4ob.spr", "brickob.spr", "rockob.spr", "snowob.spr",
        "bubbleob.spr"
    ]
    STYLE_PALETTES = [
        "level0.pal", "level1.pal", "level2.pal", "level3.pal",
        "level4.pal", "brick.pal", "rock.pal", "snow.pal",
        "bubble.pal"
    ]

    @staticmethod
    def _find_file(dir_path: str, filename: str) -> str:
        """Find a file in dir_path with case-insensitive fallback."""
        p = os.path.join(dir_path, filename)
        if os.path.exists(p):
            return p
        lower = filename.lower()
        if os.path.exists(dir_path):
            try:
                for f in os.listdir(dir_path):
                    if f.lower() == lower:
                        return os.path.join(dir_path, f)
            except OSError:
                pass
        return p

    SKILL_NAMES = [
        "climber", "floater", "bomber", "blocker",
        "builder", "basher", "miner", "digger"
    ]

    def __init__(self):
        self.release_rate = 50
        self.num_lemmings = 80
        self.to_rescue = 40
        self.time_limit = 4
        self.skills = [10] * 8
        self.start_x = 352
        self.graphic_set = 0
        self.ext_graphic_set = 0
        self.name = "Untitled Level"
        self.objects = []
        self.terrain_pieces = []
        self.steel_areas = []

    @classmethod
    def load(cls, data: bytes):
        if len(data) != 2048:
            raise ValueError(f"Invalid .LVL size {len(data)}, expected 2048")

        lvl = cls()
        # 16 big-endian words header (32 bytes)
        hdr = struct.unpack_from('>16H', data, 0)
        lvl.release_rate = hdr[0]
        lvl.num_lemmings = hdr[1]
        lvl.to_rescue = hdr[2]
        lvl.time_limit = hdr[3]
        lvl.skills = list(hdr[4:12])
        lvl.start_x = hdr[12]
        lvl.graphic_set = hdr[13]
        lvl.ext_graphic_set = hdr[14]

        # Objects: 32 entries * 8 bytes (256 bytes)
        p = 32
        lvl.objects = []
        for i in range(32):
            chunk = data[p + i * 8 : p + (i + 1) * 8]
            ox, oy, otype, oflags = struct.unpack_from('>hhHH', chunk, 0)
            if chunk != b'\x00' * 8:
                lvl.objects.append({
                    "index": i,
                    "x": ox,
                    "y": oy,
                    "type": otype,
                    "flags": oflags,
                    "raw_hex": binascii.hexlify(chunk).decode('ascii')
                })

        # Terrain: 400 entries * 4 bytes (1600 bytes)
        p += 256
        lvl.terrain_pieces = []
        for i in range(400):
            v = struct.unpack_from('>I', data, p + i * 4)[0]
            if v != 0xffffffff:
                spr_id = v & 0x3f
                x = (v & 0x1fff0000) >> 16
                if x & 0x1000:
                    x -= 0x2000
                y = (v & 0xff80) >> 7
                if y & 0x100:
                    y -= 0x200
                flags = 0
                if v & 0x80000000: flags |= 1
                if v & 0x40000000: flags |= 4
                if v & 0x20000000: flags |= 2
                lvl.terrain_pieces.append({
                    "id": spr_id,
                    "x": x,
                    "y": y,
                    "behind": bool(flags & 1),
                    "erase": bool(flags & 2),
                    "upside_down": bool(flags & 4),
                    "raw_flags": flags,
                    "raw_v": f"0x{v:08x}"
                })

        # Steel: 32 entries * 4 bytes (128 bytes)
        p += 1600
        lvl.steel_areas = []
        for i in range(32):
            v = struct.unpack_from('>I', data, p + i * 4)[0]
            if v != 0:
                x = (v & 0xff9fffff) >> 21
                y = ((v & 0x7f0000) >> 16) << 2
                w = ((v & 0xf000) >> 10) + 4
                h = ((v & 0xf00) >> 6) + 4
                lvl.steel_areas.append({
                    "index": i,
                    "x": x,
                    "y": y,
                    "width": w,
                    "height": h,
                    "raw_hex": f"0x{v:08x}"
                })

        # Title: 32 bytes at offset 2016
        name_bytes = data[2016:2048]
        lvl.name = name_bytes.rstrip(b' \x00').decode('latin1', errors='replace')
        return lvl

    def to_bytes(self) -> bytes:
        buf = bytearray(2048)
        # Header
        struct.pack_into('>HHHH', buf, 0,
                         self.release_rate, self.num_lemmings, self.to_rescue, self.time_limit)
        for i in range(8):
            sk = self.skills[i] if i < len(self.skills) else 0
            struct.pack_into('>H', buf, 8 + i * 2, sk)
        struct.pack_into('>HHH', buf, 24, self.start_x, self.graphic_set, self.ext_graphic_set)

        # Objects
        for obj in self.objects[:32]:
            idx = obj.get("index", 0)
            if "raw_hex" in obj:
                buf[32 + idx * 8 : 32 + (idx + 1) * 8] = binascii.unhexlify(obj["raw_hex"])
            else:
                struct.pack_into('>hhHH', buf, 32 + idx * 8,
                                 obj.get("x", 0), obj.get("y", 0),
                                 obj.get("type", 0), obj.get("flags", 0))

        # Terrain (default 0xffffffff empty)
        for i in range(400):
            struct.pack_into('>I', buf, 32 + 256 + i * 4, 0xffffffff)

        for i, t in enumerate(self.terrain_pieces[:400]):
            if "raw_v" in t:
                v = int(t["raw_v"], 16)
            else:
                flags = t.get("raw_flags", 0)
                if t.get("behind"): flags |= 1
                if t.get("erase"): flags |= 2
                if t.get("upside_down"): flags |= 4
                spr_id = t.get("id", 0) & 0x3f
                x = t.get("x", 0) & 0x1fff
                y = t.get("y", 0) & 0x1ff
                v = (spr_id) | (x << 16) | (y << 7)
                if flags & 1: v |= 0x80000000
                if flags & 4: v |= 0x40000000
                if flags & 2: v |= 0x20000000
            struct.pack_into('>I', buf, 32 + 256 + i * 4, v)

        # Steel
        for s in self.steel_areas[:32]:
            idx = s.get("index", 0)
            if "raw_hex" in s:
                v = int(s["raw_hex"], 16)
            else:
                x = s.get("x", 0)
                y = s.get("y", 0)
                w = max(4, s.get("width", 4))
                h = max(4, s.get("height", 4))
                v = ((x << 21) & 0xff9fffff) | (((y >> 2) & 0x7f) << 16) | (((w - 4) & 0x3c) << 10) | (((h - 4) & 0x3c) << 6)
            struct.pack_into('>I', buf, 32 + 256 + 1600 + idx * 4, v)

        # Name
        nb = self.name.encode('latin1')[:32].ljust(32, b' ')
        buf[2016:2048] = nb
        return bytes(buf)

    def to_json(self) -> str:
        return json.dumps({
            "name": self.name,
            "release_rate": self.release_rate,
            "num_lemmings": self.num_lemmings,
            "to_rescue": self.to_rescue,
            "time_limit_minutes": self.time_limit,
            "start_x": self.start_x,
            "graphic_set_id": self.graphic_set,
            "graphic_set_name": self.STYLE_NAMES[self.graphic_set] if self.graphic_set < len(self.STYLE_NAMES) else "UNKNOWN",
            "skills": dict(zip(self.SKILL_NAMES, self.skills)),
            "objects": self.objects,
            "terrain_pieces": self.terrain_pieces,
            "steel_areas": self.steel_areas
        }, indent=2)

    def render_terrain(self, styles_dir: str, scale: int = 1) -> bytes:
        """Render the complete composite terrain map to an indexed PNG."""
        set_id = self.graphic_set if self.graphic_set < len(self.STYLE_TERRAIN_SPRITES) else 0

        pal_name = self.STYLE_PALETTES[set_id]
        spr_name = self.STYLE_TERRAIN_SPRITES_HI[set_id] if scale == 2 else self.STYLE_TERRAIN_SPRITES[set_id]

        pal_path = self._find_file(styles_dir, pal_name)
        spr_path = self._find_file(styles_dir, spr_name)

        if not os.path.exists(pal_path) or not os.path.exists(spr_path):
            raise FileNotFoundError(f"Missing style graphics for set {set_id} ({spr_name}, {pal_name}) in {styles_dir}")

        with open(pal_path, 'rb') as fp:
            pal = Palette.load(fp.read())
        with open(spr_path, 'rb') as fp:
            spr_file = SpriteFile.load(fp.read())

        cw = 1664 * scale
        ch = 160 * scale
        canvas = bytearray(cw * ch)

        # Style palette colors sit from index 128..191 in WinLemm DIB
        palette_256 = [(0, 0, 0)] * 256
        for i in range(64):
            palette_256[128 + i] = pal.colors[i] if i < len(pal.colors) else (0, 0, 0)

        for t in self.terrain_pieces:
            spr_id = t["id"]
            if spr_id >= len(spr_file.frames):
                continue
            spr = spr_file.frames[spr_id]

            x = t["x"] * scale + spr.x
            y = (t["y"] - 4) * scale + spr.y
            flip = t.get("upside_down", False)
            under = t.get("behind", False)
            erase = t.get("erase", False)

            sw = spr.width
            sh = spr.height

            for r in range(sh):
                sr = (sh - 1 - r) if flip else r
                cy = y + r
                if cy < 0 or cy >= ch:
                    continue
                for c in range(sw):
                    cx = x + c
                    if cx < 0 or cx >= cw:
                        continue
                    px = spr.pixels[sr * sw + c]
                    if not px:
                        continue
                    cidx = cy * cw + cx
                    if erase:
                        canvas[cidx] = 0
                    elif under:
                        if canvas[cidx] == 0:
                            canvas[cidx] = px | 0x80
                    else:
                        canvas[cidx] = px | 0x80

        return PNG.write_indexed(cw, ch, palette_256, bytes(canvas), trans_idx=0)

    def render_objects(self, styles_dir: str, scale: int = 1) -> bytes:
        """Render level interactive objects (entrance, exit, traps, liquid) onto a transparent PNG."""
        set_id = self.graphic_set if self.graphic_set < len(self.STYLE_OBJ_SPRITES) else 0
        ground_name = self.STYLE_GROUND_FILES[set_id]
        obj_spr_name = self.STYLE_OBJ_SPRITES_HI[set_id] if scale == 2 else self.STYLE_OBJ_SPRITES[set_id]
        pal_name = self.STYLE_PALETTES[set_id]

        ground_path = self._find_file(styles_dir, ground_name)
        spr_path = self._find_file(styles_dir, obj_spr_name)
        pal_path = self._find_file(styles_dir, pal_name)

        cw = 1664 * scale
        ch = 160 * scale
        if not os.path.exists(ground_path) or not os.path.exists(spr_path) or not os.path.exists(pal_path):
            return PNG.write_indexed(cw, ch, [(0, 0, 0)] * 256, bytes(cw * ch), trans_idx=0)

        with open(ground_path, 'rb') as fp:
            ground = GroundData.load(fp.read())
        with open(spr_path, 'rb') as fp:
            spr_file = SpriteFile.load(fp.read())
        with open(pal_path, 'rb') as fp:
            pal = Palette.load(fp.read())

        canvas = bytearray(cw * ch)

        # Build sprite frame base index for each object type (CountFrames / NumberSprites)
        sprites_base = []
        next_spr = 0
        for g in ground.objects:
            cnt = g.get('end_frame', 0)
            if cnt >= 0x40:
                cnt = 0x40
            sprites_base.append(next_spr if cnt > 0 else 0)
            next_spr += cnt

        # Style palette colors sit from index 128..191 in WinLemm DIB
        palette_256 = [(0, 0, 0)] * 256
        for i in range(64):
            palette_256[128 + i] = pal.colors[i] if i < len(pal.colors) else (0, 0, 0)

        for obj in self.objects:
            otype = obj.get('type', 0)
            if otype >= len(ground.objects) or otype >= len(sprites_base):
                continue
            g = ground.objects[otype]
            frame_idx = sprites_base[otype] + g.get('start_frame', 0)
            if frame_idx >= len(spr_file.frames):
                continue
            spr = spr_file.frames[frame_idx]

            ox = obj.get('x', 0) * scale + spr.x
            oy = obj.get('y', 0) * scale + spr.y
            sw = spr.width
            sh = spr.height

            for r in range(sh):
                cy = oy + r
                if cy < 0 or cy >= ch:
                    continue
                for c in range(sw):
                    cx = ox + c
                    if cx < 0 or cx >= cw:
                        continue
                    px = spr.pixels[r * sw + c]
                    if px != 0:
                        canvas[cy * cw + cx] = px | 0x80

        return PNG.write_indexed(cw, ch, palette_256, bytes(canvas), trans_idx=0)

    def render_composite(self, styles_dir: str, scale: int = 1) -> bytes:
        """Render composite level map containing both terrain and all level objects."""
        set_id = self.graphic_set if self.graphic_set < len(self.STYLE_TERRAIN_SPRITES) else 0

        pal_name = self.STYLE_PALETTES[set_id]
        t_spr_name = self.STYLE_TERRAIN_SPRITES_HI[set_id] if scale == 2 else self.STYLE_TERRAIN_SPRITES[set_id]
        ground_name = self.STYLE_GROUND_FILES[set_id]
        o_spr_name = self.STYLE_OBJ_SPRITES_HI[set_id] if scale == 2 else self.STYLE_OBJ_SPRITES[set_id]

        pal_path = self._find_file(styles_dir, pal_name)
        t_spr_path = self._find_file(styles_dir, t_spr_name)
        ground_path = self._find_file(styles_dir, ground_name)
        o_spr_path = self._find_file(styles_dir, o_spr_name)

        if not os.path.exists(pal_path) or not os.path.exists(t_spr_path):
            raise FileNotFoundError(f"Missing terrain graphics for set {set_id} in {styles_dir}")

        with open(pal_path, 'rb') as fp:
            pal = Palette.load(fp.read())
        with open(t_spr_path, 'rb') as fp:
            t_spr_file = SpriteFile.load(fp.read())

        o_spr_file = None
        ground = None
        if os.path.exists(ground_path) and os.path.exists(o_spr_path):
            with open(ground_path, 'rb') as fp:
                ground = GroundData.load(fp.read())
            with open(o_spr_path, 'rb') as fp:
                o_spr_file = SpriteFile.load(fp.read())

        cw = 1664 * scale
        ch = 160 * scale
        canvas = bytearray(cw * ch)

        palette_256 = [(0, 0, 0)] * 256
        for i in range(64):
            palette_256[128 + i] = pal.colors[i] if i < len(pal.colors) else (0, 0, 0)

        sprites_base = []
        if ground:
            next_spr = 0
            for g in ground.objects:
                cnt = g.get('end_frame', 0)
                if cnt >= 0x40:
                    cnt = 0x40
                sprites_base.append(next_spr if cnt > 0 else 0)
                next_spr += cnt

        def draw_object_list(draw_under: bool):
            if not ground or not o_spr_file:
                return
            for obj in self.objects:
                oflags = obj.get('flags', 0)
                is_under = bool(oflags & 0x8000)
                if is_under != draw_under:
                    continue
                otype = obj.get('type', 0)
                if otype >= len(ground.objects) or otype >= len(sprites_base):
                    continue
                g = ground.objects[otype]
                frame_idx = sprites_base[otype] + g.get('start_frame', 0)
                if frame_idx >= len(o_spr_file.frames):
                    continue
                spr = o_spr_file.frames[frame_idx]
                ox = obj.get('x', 0) * scale + spr.x
                oy = obj.get('y', 0) * scale + spr.y
                sw = spr.width
                sh = spr.height
                for r in range(sh):
                    cy = oy + r
                    if cy < 0 or cy >= ch:
                        continue
                    for c in range(sw):
                        cx = ox + c
                        if cx < 0 or cx >= cw:
                            continue
                        px = spr.pixels[r * sw + c]
                        if px != 0:
                            if draw_under and canvas[cy * cw + cx] != 0:
                                continue
                            canvas[cy * cw + cx] = px | 0x80

        # 1. Under objects
        draw_object_list(draw_under=True)

        # 2. Terrain
        for t in self.terrain_pieces:
            spr_id = t["id"]
            if spr_id >= len(t_spr_file.frames):
                continue
            spr = t_spr_file.frames[spr_id]
            x = t["x"] * scale + spr.x
            y = (t["y"] - 4) * scale + spr.y
            flip = t.get("upside_down", False)
            under = t.get("behind", False)
            erase = t.get("erase", False)
            sw = spr.width
            sh = spr.height
            for r in range(sh):
                sr = (sh - 1 - r) if flip else r
                cy = y + r
                if cy < 0 or cy >= ch:
                    continue
                for c in range(sw):
                    cx = x + c
                    if cx < 0 or cx >= cw:
                        continue
                    px = spr.pixels[sr * sw + c]
                    if not px:
                        continue
                    cidx = cy * cw + cx
                    if erase:
                        canvas[cidx] = 0
                    elif under:
                        if canvas[cidx] == 0:
                            canvas[cidx] = px | 0x80
                    else:
                        canvas[cidx] = px | 0x80

        # 3. Foreground objects
        draw_object_list(draw_under=False)

        return PNG.write_indexed(cw, ch, palette_256, bytes(canvas), trans_idx=0)


# ============================================================================
# High-Level CLI Subcommands
# ============================================================================

def cmd_unpack_spr(args):
    with open(args.input, 'rb') as fp:
        spr = SpriteFile.load(fp.read())
    pal = None
    if args.pal:
        with open(args.pal, 'rb') as fp:
            pal = Palette.load(fp.read())
    spr.extract(args.output, palette=pal, make_sheet=args.sheet)
    print(f"Extracted {len(spr.frames)} frames from {args.input} to {args.output}/")


def cmd_pack_spr(args):
    pal = None
    if args.pal:
        with open(args.pal, 'rb') as fp:
            pal = Palette.load(fp.read())
    spr = SpriteFile.from_dir(args.input, palette=pal)
    out_bytes = spr.to_bytes()
    with open(args.output, 'wb') as fp:
        fp.write(out_bytes)
    print(f"Packed {len(spr.frames)} frames from {args.input}/ into {args.output} ({len(out_bytes)} bytes)")


def cmd_unpack_pal(args):
    with open(args.input, 'rb') as fp:
        pal = Palette.load(fp.read())
    if args.output.lower().endswith('.png'):
        png_data = pal.render_swatch()
        with open(args.output, 'wb') as fp:
            fp.write(png_data)
        print(f"Exported palette swatch image to {args.output}")
    else:
        with open(args.output, 'w') as fp:
            fp.write(pal.to_json())
        print(f"Exported palette JSON to {args.output}")


def cmd_pack_pal(args):
    with open(args.input, 'r') as fp:
        pal = Palette.from_json(fp.read())
    with open(args.output, 'wb') as fp:
        fp.write(pal.to_bytes())
    print(f"Compiled palette {args.input} into {args.output}")


def cmd_unpack_dat(args):
    with open(args.input, 'rb') as fp:
        g = GroundData.load(fp.read())
    with open(args.output, 'w') as fp:
        fp.write(g.to_json())
    print(f"Extracted {len(g.objects)} object definitions from {args.input} to {args.output}")


def cmd_pack_dat(args):
    with open(args.input, 'r') as fp:
        g = GroundData.from_json(fp.read())
    with open(args.output, 'wb') as fp:
        fp.write(g.to_bytes())
    print(f"Compiled {len(g.objects)} object definitions into {args.output}")


def cmd_render_lvl(args):
    with open(args.input, 'rb') as fp:
        lvl = LevelFile.load(fp.read())
    png_bytes = lvl.render_terrain(args.styles_dir, scale=args.scale)
    with open(args.output, 'wb') as fp:
        fp.write(png_bytes)
    print(f"Rendered level map '{lvl.name.strip()}' ({lvl.graphic_set}) to {args.output}")
    if args.json:
        with open(args.json, 'w') as fp:
            fp.write(lvl.to_json())
        print(f"Exported level metadata to {args.json}")


def cmd_pack_lvl(args):
    with open(args.input, 'r') as fp:
        data = json.load(fp)
    lvl = LevelFile()
    lvl.name = data.get("name", "Untitled")
    lvl.release_rate = data.get("release_rate", 50)
    lvl.num_lemmings = data.get("num_lemmings", 80)
    lvl.to_rescue = data.get("to_rescue", 40)
    lvl.time_limit = data.get("time_limit_minutes", 4)
    lvl.start_x = data.get("start_x", 352)
    lvl.graphic_set = data.get("graphic_set_id", 0)
    if "skills" in data:
        lvl.skills = [data["skills"].get(k, 0) for k in LevelFile.SKILL_NAMES]
    lvl.objects = data.get("objects", [])
    lvl.terrain_pieces = data.get("terrain_pieces", [])
    lvl.steel_areas = data.get("steel_areas", [])

    with open(args.output, 'wb') as fp:
        fp.write(lvl.to_bytes())
    print(f"Compiled level '{lvl.name}' into {args.output} (2048 bytes)")


def cmd_dump_all(args):
    """Batch export all game assets from orig/WINLEMM/ to export_dir/."""
    winroot = args.winroot
    outdir = args.output
    os.makedirs(outdir, exist_ok=True)

    print(f"=== Dumping All Assets from {winroot} to {outdir} ===")

    # 1. Palettes
    pal_dir = os.path.join(outdir, "palettes")
    os.makedirs(pal_dir, exist_ok=True)
    pals = {}
    for ppath in sorted(glob.glob(os.path.join(winroot, "**", "*.PAL"), recursive=True)):
        bname = os.path.splitext(os.path.basename(ppath))[0]
        with open(ppath, 'rb') as fp:
            pal = Palette.load(fp.read())
        pals[bname.lower()] = pal
        with open(os.path.join(pal_dir, f"{bname}.json"), 'w') as fp:
            fp.write(pal.to_json())
        with open(os.path.join(pal_dir, f"{bname}_swatch.png"), 'wb') as fp:
            fp.write(pal.render_swatch())
    print(f"Dumped {len(pals)} palettes.")

    # 2. Sprites
    spr_dir = os.path.join(outdir, "sprites")
    os.makedirs(spr_dir, exist_ok=True)
    count_spr = 0
    for spath in sorted(glob.glob(os.path.join(winroot, "**", "*.SPR"), recursive=True)):
        with open(spath, 'rb') as fp:
            data = fp.read()
        if data.startswith(b'BMPSpritePos'):
            continue
        bname = os.path.splitext(os.path.basename(spath))[0]
        spr = SpriteFile.load(data)
        # Choose matching palette
        chosen_pal = pals.get("fe")
        for k in pals:
            if k in bname.lower():
                chosen_pal = pals[k]
                break
        spr.extract(os.path.join(spr_dir, bname), palette=chosen_pal, make_sheet=True)
        count_spr += 1
    print(f"Dumped {count_spr} sprite sets.")

    # 3. Ground DATs
    dat_dir = os.path.join(outdir, "ground")
    os.makedirs(dat_dir, exist_ok=True)
    count_dat = 0
    for dpath in sorted(glob.glob(os.path.join(winroot, "STYLES", "*.DAT"))):
        bname = os.path.splitext(os.path.basename(dpath))[0]
        with open(dpath, 'rb') as fp:
            g = GroundData.load(fp.read())
        with open(os.path.join(dat_dir, f"{bname}.json"), 'w') as fp:
            fp.write(g.to_json())
        count_dat += 1
    print(f"Dumped {count_dat} ground object definition files.")

    # 4. Levels & Terrain Maps
    lvl_dir = os.path.join(outdir, "levels")
    os.makedirs(lvl_dir, exist_ok=True)
    styles_dir = os.path.join(winroot, "STYLES")
    count_lvl = 0
    for lpath in sorted(glob.glob(os.path.join(winroot, "LEVEL", "**", "*.LVL"), recursive=True)):
        cat = os.path.basename(os.path.dirname(lpath))
        bname = os.path.splitext(os.path.basename(lpath))[0]
        target_dir = os.path.join(lvl_dir, cat)
        os.makedirs(target_dir, exist_ok=True)
        with open(lpath, 'rb') as fp:
            lvl = LevelFile.load(fp.read())
        with open(os.path.join(target_dir, f"{bname}.json"), 'w') as fp:
            fp.write(lvl.to_json())
        try:
            png_bytes = lvl.render_terrain(styles_dir, scale=1)
            with open(os.path.join(target_dir, f"{bname}_map.png"), 'wb') as fp:
                fp.write(png_bytes)
        except Exception as e:
            print(f"Warning: could not render terrain for {bname}: {e}")
        count_lvl += 1
    print(f"Dumped and rendered {count_lvl} levels.")
    print(f"All assets dumped successfully to {outdir}/!")


def cmd_gui(args):
    """Launch the interactive asset viewer GUI."""
    # Check if the web GUI repository is adjacent
    gui_repo = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "..", "Lemmings-Web-Player")
    if os.path.isdir(gui_repo) and gui_repo not in sys.path:
        sys.path.insert(0, os.path.abspath(gui_repo))

    try:
        from asset_gui import run_gui
        run_gui(winroot=args.winroot, port=args.port, open_browser=not args.no_browser)
    except ImportError:
        print("Note: The Asset Studio & Web Level Player has been moved to its own repository:")
        print("  https://github.com/NitrosGits/lemmings-web-player")
        print("You can run it via:")
        print("  cd ../Lemmings-Web-Player && python3 app.py " + args.winroot)
        sys.exit(1)


def cmd_verify(args):
    """Run round-trip verification across all .SPR files in winroot."""
    winroot = args.winroot
    spr_files = sorted(glob.glob(os.path.join(winroot, "**", "*.SPR"), recursive=True))
    total = 0
    exact = 0
    skipped = 0

    print(f"Running Sprite RLE Round-Trip Verification on {len(spr_files)} files...")
    for f in spr_files:
        with open(f, 'rb') as fp:
            orig = fp.read()
        if orig.startswith(b'BMPSpritePos'):
            skipped += 1
            continue
        total += 1
        spr = SpriteFile.load(orig)
        rebuilt = spr.to_bytes()
        if orig == rebuilt:
            exact += 1
        else:
            print(f"MISMATCH: {f} (orig {len(orig)} != re {len(rebuilt)})")

    print(f"Verification Results:")
    print(f"  Total RLE files verified : {total}")
    print(f"  Exact bit-for-bit matches: {exact} ({100.0 * exact / total:.1f}%)")
    print(f"  Legacy BMPSpritePos files: {skipped}")
    if exact == total:
        print("SUCCESS: 100% Bit-Identical Asset Encoding Verified!")


def main():
    parser = argparse.ArgumentParser(description="Lemmings 95 Asset Tool (Pure Python)")
    subparsers = parser.add_subparsers(dest="command", required=True)

    # unpack-spr
    p_uspr = subparsers.add_parser("unpack-spr", help="Unpack .SPR to frame PNGs and metadata.json")
    p_uspr.add_argument("input", help="Path to input .SPR file")
    p_uspr.add_argument("-o", "--output", required=True, help="Output directory")
    p_uspr.add_argument("--pal", help="Optional .PAL file for coloring indexed frames")
    p_uspr.add_argument("--sheet", action="store_true", default=True, help="Generate composite spritesheet.png")

    # pack-spr
    p_pspr = subparsers.add_parser("pack-spr", help="Pack directory of frames into .SPR")
    p_pspr.add_argument("input", help="Input directory containing metadata.json and frame images")
    p_pspr.add_argument("-o", "--output", required=True, help="Output .SPR file")
    p_pspr.add_argument("--pal", help="Optional .PAL palette for color quantization")

    # unpack-pal
    p_upal = subparsers.add_parser("unpack-pal", help="Export .PAL to JSON or swatch PNG")
    p_upal.add_argument("input", help="Path to input .PAL file")
    p_upal.add_argument("-o", "--output", required=True, help="Output .json or .png swatch")

    # pack-pal
    p_ppal = subparsers.add_parser("pack-pal", help="Compile JSON palette into .PAL")
    p_ppal.add_argument("input", help="Input JSON palette file")
    p_ppal.add_argument("-o", "--output", required=True, help="Output .PAL file")

    # unpack-dat
    p_udat = subparsers.add_parser("unpack-dat", help="Export GROUND*.DAT objects to JSON")
    p_udat.add_argument("input", help="Path to input GROUND*.DAT file")
    p_udat.add_argument("-o", "--output", required=True, help="Output JSON file")

    # pack-dat
    p_pdat = subparsers.add_parser("pack-dat", help="Compile JSON object definitions into GROUND*.DAT")
    p_pdat.add_argument("input", help="Input JSON file")
    p_pdat.add_argument("-o", "--output", required=True, help="Output .DAT file")

    # render-lvl
    p_rlvl = subparsers.add_parser("render-lvl", help="Render .LVL level terrain to PNG and export JSON")
    p_rlvl.add_argument("input", help="Path to input .LVL file")
    p_rlvl.add_argument("-o", "--output", required=True, help="Output terrain map PNG")
    p_rlvl.add_argument("--styles-dir", default="orig/WINLEMM/STYLES", help="Path to STYLES/ directory")
    p_rlvl.add_argument("--scale", type=int, choices=[1, 2], default=1, help="1 for standard (1664x160), 2 for high-res")
    p_rlvl.add_argument("--json", help="Optional path to export level metadata JSON")

    # pack-lvl
    p_plvl = subparsers.add_parser("pack-lvl", help="Compile JSON level definition into 2048-byte .LVL")
    p_plvl.add_argument("input", help="Input JSON level file")
    p_plvl.add_argument("-o", "--output", required=True, help="Output .LVL file")

    # dump-all
    p_dump = subparsers.add_parser("dump-all", help="Batch extract all assets from WINLEMM into a folder")
    p_dump.add_argument("winroot", default="orig/WINLEMM", nargs="?", help="Path to orig/WINLEMM root")
    p_dump.add_argument("-o", "--output", default="build/extracted_assets", help="Output directory")

    # verify
    p_ver = subparsers.add_parser("verify", help="Verify round-trip encoding on all .SPR files")
    p_ver.add_argument("winroot", default="orig/WINLEMM", nargs="?", help="Path to orig/WINLEMM root")

    # gui
    p_gui = subparsers.add_parser("gui", help="Launch interactive Asset Studio & Viewer GUI")
    p_gui.add_argument("winroot", default="orig/WINLEMM", nargs="?", help="Path to orig/WINLEMM root")
    p_gui.add_argument("--port", type=int, default=8080, help="Port to serve web GUI (default: 8080)")
    p_gui.add_argument("--no-browser", action="store_true", help="Do not open web browser automatically")

    args = parser.parse_args()

    dispatch = {
        "unpack-spr": cmd_unpack_spr,
        "pack-spr": cmd_pack_spr,
        "unpack-pal": cmd_unpack_pal,
        "pack-pal": cmd_pack_pal,
        "unpack-dat": cmd_unpack_dat,
        "pack-dat": cmd_pack_dat,
        "render-lvl": cmd_render_lvl,
        "pack-lvl": cmd_pack_lvl,
        "dump-all": cmd_dump_all,
        "verify": cmd_verify,
        "gui": cmd_gui,
    }

    dispatch[args.command](args)


if __name__ == '__main__':
    main()

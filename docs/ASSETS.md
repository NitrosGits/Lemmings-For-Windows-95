# Lemmings for Windows 95 - Asset Formats & Custom Content Guide

This document describes the binary asset formats used by *Lemmings for Windows 95* (`WINLEMM.EXE` and associated runtime DLLs), as well as instructions for using the standalone asset pipeline (`tools/asset_tool.py`) to inspect, extract, author, and package replacement non-copyrighted assets.

---

## 1. Overview of Game Assets

The original game loads its data files from the following subdirectories:

| Directory | File Type | Format Description |
|---|---|---|
| `GFX/` | `*.SPR` | Lemming animations, HUD icons, UI buttons, and fonts stored in DMA Design's Run-Length Encoded (RLE) format (`SRLE`). |
| `GFX/` | `FE.PAL` | 256-color palette (`.PAL`) used by the Windows front-end and main menus. |
| `STYLES/` | `*.SPR` | Terrain tiles (`*S.SPR` low-res 1x, `*B.SPR` high-res 2x) and animated level objects (`*OS.SPR`, `*OB.SPR`). |
| `STYLES/` | `*.PAL` | 64-color style palettes (`LEVEL0.PAL` through `BUBBLE.PAL`) mapped to DIB palette indices 128..191. |
| `STYLES/` | `*.DAT` | Interactive object definitions (`GROUND0O.DAT` through `BUBBLE.DAT`): entrances, exits, traps, and sounds. |
| `LEVEL/` | `*.LVL` | Big-endian 2048-byte level files containing level stats, terrain blit lists, steel zones, and initial skills. |
| `LEVEL/` | `*.BMP` | 200x40 16-color (4-bit) preview thumbnails displayed in the level selection dialog. |
| `SOUND/` | `*.WAV` | Standard 8-bit / 16-bit uncompressed PCM Wave sound effects (e.g., `LEMETSGO.WAV`, `YIPPEE.WAV`, `OHNO.WAV`). |
| `MUSIC/` | `*.MID` | Standard MIDI files (Format 0/1) for in-game and title music tracks. |
| `USERS/` | `*.DB` | Binary records storing player profiles, completed levels, and best times. |

---

## 2. Binary Specifications

### 2.1 Sprite Files (`.SPR`)
All runtime sprite files begin with a 6-byte header followed by an offset table:

```
[0x00..0x03]  char magic[4]     : "SRLE" (0x454C5253 in little-endian)
[0x04..0x05]  WORD frameCount   : Number of frames in the file
[0x06..     ]  DWORD offsets[]   : File offsets for each frame (relative to start of file)
```

Each frame at offset `offsets[i]` begins with a 12-byte frame header:
```
[0x00..0x01]  WORD x            : Hotspot / draw X offset relative to placement position
[0x02..0x03]  WORD y            : Hotspot / draw Y offset relative to placement position
[0x04..0x05]  WORD width        : Pixel width of the frame
[0x06..0x07]  WORD height       : Pixel height of the frame
[0x08..0x09]  WORD canvasWidth  : Original canvas bounding width
[0x0A..0x0B]  WORD canvasHeight : Original canvas bounding height
[0x0C..     ]  BYTE rleData[]    : RLE compressed scanlines
```

#### RLE Scanline Encoding:
Scanlines are encoded sequentially from row 0 to `height - 1`:
- `0x80`: End of current scanline. Any remaining pixels across the row's `width` remain transparent (index 0).
- `0x81 .. 0xFF`: Literal run. The lower 7 bits `(byte & 0x7F)` indicate the count `N` of opaque pixel palette indices immediately following.
- `0x00 .. 0x7F`: Transparent skip. Skips `byte` transparent pixels (index 0).

*Note: 5 legacy files in `STYLES/` (`LEVEL0O.SPR` through `LEVEL4O.SPR`) begin with the magic string `BMPSpritePos` rather than `SRLE`. These are obsolete development-time slicing tables from 1995; the game runtime instead loads `LEVEL0OS.SPR` / `LEVEL0OB.SPR`.*

---

### 2.2 Palette Files (`.PAL`)
Palette files store color lookup tables:
```
[0x00..0x03]  DWORD magic       : 0x50414C20 (" PAL" in little-endian)
[0x04..0x05]  WORD count        : Color count (256 for FE.PAL; 64 for style palettes)
[0x06..     ]  BYTE colors[][4]  : count * 4 bytes of (Red, Green, Blue, 0x00)
```

In the in-game terrain DIB:
- Color index `0` represents transparent background.
- Indices `128 .. 191` store the 64 colors loaded from the level's style palette.
- Remaining indices are used for UI controls and Lemming sprites.

---

### 2.3 Interactive Ground Data (`.DAT`)
Each `GROUND*.DAT` file in `STYLES/` contains sixteen 28-byte object descriptors followed by 608 bytes of legacy terrain descriptors:

```c
typedef struct GROUNDOBJ {
    BYTE flags;          // Modifier flags (0=normal, 1=animated trap, 2=door, etc.)
    BYTE flags2;
    BYTE startFrame;     // First animation frame index in *OS.SPR / *OB.SPR
    BYTE endFrame;       // Total frames in animation sequence
    BYTE width;          // Sprite pixel width
    BYTE height;         // Sprite pixel height
    BYTE other[8];
    WORD trigLeft;       // Trigger collision box X offset
    WORD trigTop;        // Trigger collision box Y offset
    BYTE trigWidth;      // Trigger collision box width
    BYTE trigHeight;     // Trigger collision box height
    BYTE trigEffect;     // Action triggered (kill, exit, trap effect)
    BYTE other2[6];
    BYTE sound;          // Sound index triggered upon activation
} GROUNDOBJ;
```

---

### 2.4 Level Files (`.LVL`)
Level files are exactly 2048 bytes long and stored in big-endian (Motorola 68000 format inherited from the original Commodore Amiga codebase):

```
Offset       Size   Field
-------------------------------------------------------------------------------
0x0000..0x01   2    Release Rate (0..99, where 99 is fastest)
0x0002..0x03   2    Total Lemmings in level (typically 1..100)
0x0004..0x05   2    Number of Lemmings required to rescue
0x0006..0x07   2    Time limit in minutes
0x0008..0x17  16    Skill counts (8 big-endian 16-bit words):
                    Climbers, Floaters, Bombers, Blockers,
                    Builders, Bashers, Miners, Diggers
0x0018..0x19   2    Initial Camera Start X (horizontal scroll position)
0x001A..0x1B   2    Graphic Set / Style (0=Dirt, 1=Fire, 2=Marble, 3=Pillar,
                    4=Crystal, 5=Brick, 6=Rock, 7=Snow, 8=Bubble)
0x001C..0x1D   2    Extended Graphic Set flag
0x001E..0x1F   2    Unused / Padding
0x0020..0x11F 256   Objects list: 32 entries * 8 bytes (x, y, object_type, flags)
0x0120..0x75F 1600  Terrain pieces: 400 entries * 4 bytes (encoded DWORDs)
0x0760..0x7DF 128   Steel zones: 32 entries * 4 bytes
0x07E0..0x7FF  32   Level title (ASCII string, space-padded, null-terminated)
```

#### Terrain Piece 32-bit DWORD Encoding:
- **Bits 29..31 (`v >> 29`)**: Rendering flags
  - Bit 0 (`flags & 1`): Draw Behind / Under (only renders pixels where terrain buffer is 0)
  - Bit 1 (`flags & 2`): Erase / Negative (erases terrain pixels)
  - Bit 2 (`flags & 4`): Invert / Upside-Down (vertically flipped)
- **Bits 23..28 (`(v >> 23) & 0x3F`)**: Terrain piece ID (index into style `.SPR`)
- **Bits 10..18 (`((v >> 10) & 0x1FF) - 16`)**: X coordinate on level canvas
- **Bits 0..9 (`(v & 0x3FF) - 40`)**: Y coordinate on level canvas

---

## 3. The `tools/asset_tool.py` Utility

The project includes a pure Python (zero external dependencies) command-line tool for reading, editing, and repacking all game assets.

### 3.1 Unpack Sprites to PNG
To unpack a sprite file into individual frame PNGs, a composite sprite sheet, and a `metadata.json` descriptor:

```bash
python3 tools/asset_tool.py unpack-spr orig/WINLEMM/GFX/LEMMBIG.SPR -o build/extracted/lemmbig --pal orig/WINLEMM/GFX/FE.PAL
```

This creates:
- `frame_000.png`, `frame_001.png`, ...
- `spritesheet.png` (filmstrip of all frames for visual reference)
- `metadata.json` (stores frame dimensions, hotspot offsets, and original bounding box)

### 3.2 Pack PNG Frames Back to `.SPR`
To re-encode PNG frames into a byte-accurate binary `.SPR` file:

```bash
python3 tools/asset_tool.py pack-spr build/extracted/lemmbig -o GFX/LEMMBIG.SPR --pal orig/WINLEMM/GFX/FE.PAL
```

### 3.3 Render Level Maps
To render a full 1664x160 composite terrain image and JSON statistics from any `.LVL` file:

```bash
python3 tools/asset_tool.py render-lvl orig/WINLEMM/LEVEL/ORIG/LVL0000.LVL \
    -o build/level0000_map.png \
    --styles-dir orig/WINLEMM/STYLES/ \
    --json build/level0000.json
```

### 3.4 Compile Custom Levels
To compile a modified or newly created level JSON file back into the exact 2048-byte `.LVL` binary format:

```bash
python3 tools/asset_tool.py pack-lvl custom_level.json -o LEVEL/ORIG/LVL0000.LVL
```

### 3.5 Export All Assets (Batch Dump)
To extract every palette, sprite sheet, ground object definition, and rendered level map across the entire game into a folder in seconds:

```bash
python3 tools/asset_tool.py dump-all orig/WINLEMM -o build/all_assets/
```

### 3.6 Interactive Asset Studio & Viewer GUI
To launch the interactive GUI with real-time animation playback, level map inspection, palette swatches, and soundboard:

```bash
python3 tools/asset_tool.py gui
# or directly:
python3 tools/asset_gui.py
```
This starts a local zero-dependency web application (defaults to `http://127.0.0.1:8080`) and automatically opens your browser.

#### GUI Features:
- **Sprite Animation Studio**:
  - Live playback controls (Play, Pause, Step Forward/Backward).
  - Variable playback speed (1 to 60 FPS) and frame scrubber.
  - Nearest-neighbor pixelated zoom (1x to 8x).
  - Background modes: Transparency Checkerboard, Pitch Black, Green Screen, White.
  - Filmstrip view showing all individual frame thumbnails with coordinates and bounding boxes.
  - One-click downloads for single frame PNGs and composite sprite sheets.
- **Level Map Inspector**:
  - Full-resolution pan/zoom of the 1664x160 composite level terrain.
  - Toggleable overlays:
    - Terrain layer
    - Steel obstacle zones (highlighted with collision rectangles)
    - Interactive objects (Entrance hatches, exit doors, animated traps)
    - Camera start position indicator (X scroll marker)
  - Level stats summary card (Lemmings, rescue quotas, release rate, time, skills).
  - Direct 1-click PNG export of the full composite map.
- **Color Palette Inspector**:
  - Visual color swatch grid for all 10 frontend and style palettes.
  - Click any color swatch to copy its `#RRGGBB` hex value to clipboard.
- **Audio Soundboard**:
  - Interactive playback of all 24 classic WAV sound effects (`LEMETSGO.WAV`, `OHNO.WAV`, `YIPPEE.WAV`, etc.).

---

### 3.7 Verify Bit-for-Bit RLE Encoding
To verify that the custom RLE compressor matches DMA Design's original compressor:

```bash
python3 tools/asset_tool.py verify orig/WINLEMM
```
*(Confirms 100% bit-for-bit equivalence across all 62 runtime sprite sets).*

---

## 4. Building an Open-Asset Replacement Pack

To run the decompiled binary completely free of proprietary Psygnosis/DMA Design intellectual property, replacement assets can be constructed using open formats:

1. **Sprites (`GFX/*.SPR`)**:
   - Draw open-license sprites in any graphics editor (Aseprite, GIMP, Photoshop, Paint.NET).
   - Export frames as 8-bit indexed PNG or 32-bit RGBA PNG (transparent background).
   - Use `python3 tools/asset_tool.py pack-spr` to generate the `.SPR` files.

2. **Audio (`SOUND/*.WAV`)**:
   - Record or synthesize public domain / Creative Commons 0 (CC0) voice lines ("Let's go!", "Oh no!", "Yippee!").
   - Export as uncompressed 11025 Hz or 22050 Hz PCM `.WAV`.

3. **Music (`MUSIC/*.MID`)**:
   - Compose or transcribe royalty-free MIDI sequences and save as standard `.MID` files matching the names in `orig/WINLEMM/MUSIC/`.

4. **Levels (`LEVEL/*/*.LVL`)**:
   - Design original puzzle layouts using JSON or compatible level editors and compile with `python3 tools/asset_tool.py pack-lvl`.

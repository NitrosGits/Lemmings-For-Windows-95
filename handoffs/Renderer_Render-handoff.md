# Renderer_Render hand-off

> Generated 2026-11-04. Project `/Users/daniel/Lemmings-For-Windows-95` (not a git repo). Module **RENDER.DLL**, function at **0x10001020**, source `src/render/render.c`.
> Status: `0x10001020 Renderer_Render: DIFF (896 bytes) 43 bytes differ, first at +0x66, 5 insns differ; +0x74 _g_renderers$S19193: original 0xc82b890c, expected 0x1000f890; +0x7b _g_canvases$S19192: original 0x3304fbc1, expected 0x10007000`

## Goal
Make `Renderer_Render` compile with VC++ 2.2 (`/Ox`, via `tools/msvc/try.sh RENDER.DLL src/render/render.c -v -f Renderer_Render`) to bytes identical to the original, without breaking the other functions in `render.c` or the MinGW build (`make`).

## How to test
```
zsh tools/msvc/try.sh RENDER.DLL src/render/render.c -v -f Renderer_Render
python3 tools/msvc/realname.py RENDER.DLL src/render/render.c 0x10001020 SPEC.py --jobs 3   # sweep source variants under the real file name
```
`realname.py`'s insn count can hide byte growth: always confirm with `try.sh`. See `docs/DECOMP.md` (padding externs, size-mode functions, block-scoped temporaries, inline-member expansion) and HANDOFF.md.

## Current source
```cpp
/* 0x10001020 */
void WINAPI Renderer_Render(WORD renderer, HDC dc, WORD slot, int left, int top, int right,
                            int bottom, int width, int height, int mode)
{
    DWORD end;
    CANVAS *c;
    int dy;
    HDC sdc;
    DWORD start;
    int r;
    int across;
    int down;
    HGDIOBJ old;
    SLOT *s;
    int dx;
    BYTE *changed;

    if (renderer == 0xffff)
        return;
    r = (WORD)(renderer ^ HANDLE_XOR);
    s = &g_renderers[r].slots[slot];
    c = &g_canvases[s->canvas];
    dx = 0;
    changed = SLOT_OF(r, slot).changed;
    down = (SLOT_OF(r, slot).height + 7) >> 3;
    across = (SLOT_OF(r, slot).width + 15) >> 4;
    dy = 0;
    if (left & 15) {
        dx = -(left & 15);
        left &= 0xfff0;
    }
    if (top & 7) {
        dy = -(top & 7);
        top &= 0xfff8;
    }
    if (s->flags & SLOT_SCREEN)
        sdc = s->dc;
    else
        MessageBoxA(NULL, "We should not get this", "RENDER_RENDER", MB_OK);
    old = SelectObject(sdc, CANVAS_OF(r, slot).bitmap);
    switch (mode) {
    case 0:
        if (right > across * 16)
            right = across * 16;
        if (bottom > down * 8)
            bottom = down * 8;
        start = timeGetTime();
        BitBlt(dc, dx, 0, right - left + 1, bottom - top + 1, sdc, left, top, SRCCOPY);
        end = timeGetTime();
        break;
    case 1:
        if (right > across * 16)
            right = across * 16;
        if (bottom > down * 8)
            bottom = down * 8;
        start = timeGetTime();
        CopyChanged(dc, dx, dy, sdc, left, top, right, bottom, changed, across, down, c->bits,
                    (BITMAPINFO *)&c->header);
        end = timeGetTime();
        break;
    case 2:
        if (right > across * 16)
            right = across * 16;
        if (bottom > down * 8)
            bottom = down * 8;
        start = timeGetTime();
        StretchChanged(dc, dx, dy, sdc, left, top, right, bottom, changed, across, down, width, height,
                       c->bits, (BITMAPINFO *)&c->header);
        end = timeGetTime();
        break;
    case 3:
        if (right > across * 16)
            right = across * 16;
        if (bottom > down * 8)
            bottom = down * 8;
        start = timeGetTime();
        StretchAll(dc, sdc, left, top, right, bottom, changed, across, down, width, height);
        end = timeGetTime();
        break;
    }
    g_blitTime += end - start;
    GdiFlush();
    if (s->flags & SLOT_SCREEN) {
        start = timeGetTime();
        RestoreChanged(r, slot);
        g_copyTime += timeGetTime() - start;
    }
    NextFrame(r, slot);
    SelectObject(sdc, old);
}
```

## Side-by-side (ours | original; `*` marks differing rows)
```
     10001020  mov    ax,WORD PTR [esp+0x4]                  | mov    ax,WORD PTR [esp+0x4]
     10001025  sub    esp,0x24                               | sub    esp,0x24
     10001028  cmp    ax,0xffff                              | cmp    ax,0xffff
     1000102c  push   ebx                                    | push   ebx
     1000102d  push   esi                                    | push   esi
     1000102e  push   edi                                    | push   edi
     1000102f  push   ebp                                    | push   ebp
     10001030  je     0x10001379                             | je     0x10001379
     10001036  xor    ax,0x55aa                              | xor    ax,0x55aa
     1000103a  movzx  ecx,ax                                 | movzx  ecx,ax
     1000103d  mov    eax,DWORD PTR [esp+0x40]               | mov    eax,DWORD PTR [esp+0x40]
     10001041  mov    DWORD PTR [esp+0x14],ecx               | mov    DWORD PTR [esp+0x14],ecx
     10001045  shl    ecx,0x3                                | shl    ecx,0x3
     10001048  mov    edx,DWORD PTR [esp+0x14]               | mov    edx,DWORD PTR [esp+0x14]
     1000104c  sub    ecx,DWORD PTR [esp+0x14]               | sub    ecx,DWORD PTR [esp+0x14]
     10001050  and    eax,0xffff                             | and    eax,0xffff
     10001055  lea    ebx,[edx+ecx*4]                        | lea    ebx,[edx+ecx*4]
     10001058  lea    ecx,[edx+ebx*2]                        | lea    ecx,[edx+ebx*2]
     1000105b  add    ecx,eax                                | add    ecx,eax
     1000105d  lea    edx,[ecx+ecx*2]                        | lea    edx,[ecx+ecx*2]
     10001060  xor    ecx,ecx                                | xor    ecx,ecx
     10001062  lea    eax,[edx*8+0x1000f88c]                 | lea    eax,[edx*8+0x1000f88c]
     10001069  lea    esi,[edx*8+0x0]                        | lea    esi,[edx*8+0x0]
     10001070  mov    cx,WORD PTR [eax+0x6]                  | mov    cx,WORD PTR [eax+0x6]
     10001074  mov    DWORD PTR [esp+0x18],eax               | mov    DWORD PTR [esp+0x18],eax
     10001078  mov    eax,ecx                                | mov    eax,ecx
     1000107a  lea    edx,[ecx+ecx*8]                        | lea    edx,[ecx+ecx*8]
     1000107d  lea    ecx,[eax+edx*8]                        | lea    ecx,[eax+edx*8]
     10001080  mov    edx,DWORD PTR [esi+0x1000f8a0]         | mov    edx,DWORD PTR [esi+0x1000f8a0]
   * 10001086  mov    DWORD PTR [esp+0x28],edx               | xor    ebx,ebx
   * 1000108a  lea    ecx,[ecx+ecx*4]                        | mov    DWORD PTR [esp+0x28],edx
   * 1000108d  sub    ecx,eax                                | mov    bx,WORD PTR [esi+0x1000f88e]
   * 1000108f  xor    ebp,ebp                                | lea    ecx,[ecx+ecx*4]
   * 10001091  mov    bp,WORD PTR [esi+0x1000f890]           | sub    ecx,eax
   * 10001098  lea    eax,[ecx+ecx*2+0x10007000]             | add    ebx,0xf
   * 1000109f  xor    ecx,ecx                                | sar    ebx,0x4
   * 100010a1  add    ebp,0x7                                | xor    ebp,ebp
   * 100010a4  sar    ebp,0x3                                | mov    bp,WORD PTR [esi+0x1000f890]
   * 100010a7  xor    ebx,ebx                                | lea    eax,[ecx+ecx*2+0x10007000]
   * 100010a9  mov    bx,WORD PTR [esi+0x1000f88e]           | add    ebp,0x7
   * 100010b0  mov    DWORD PTR [esp+0x24],eax               | xor    ecx,ecx
   * 100010b4  add    ebx,0xf                                | sar    ebp,0x3
   * 100010b7  mov    eax,DWORD PTR [esp+0x44]               | mov    DWORD PTR [esp+0x24],eax
   * 100010bb  sar    ebx,0x4                                | mov    eax,DWORD PTR [esp+0x44]
     100010be  mov    DWORD PTR [esp+0x20],ecx               | mov    DWORD PTR [esp+0x20],ecx
     100010c2  and    eax,0xf                                | and    eax,0xf
     100010c5  mov    DWORD PTR [esp+0x2c],ecx               | mov    DWORD PTR [esp+0x2c],ecx
     100010c9  je     0x100010d9                             | je     0x100010d9
     100010cb  neg    eax                                    | neg    eax
     100010cd  and    DWORD PTR [esp+0x44],0xfff0            | and    DWORD PTR [esp+0x44],0xfff0
     100010d4                                                | 
     100010d5  mov    DWORD PTR [esp+0x20],eax               | mov    DWORD PTR [esp+0x20],eax
     100010d9  mov    eax,DWORD PTR [esp+0x48]               | mov    eax,DWORD PTR [esp+0x48]
     100010dd  and    eax,0x7                                | and    eax,0x7
     100010e0  je     0x100010f0                             | je     0x100010f0
     100010e2  neg    eax                                    | neg    eax
     100010e4  and    DWORD PTR [esp+0x48],0xfff8            | and    DWORD PTR [esp+0x48],0xfff8
     100010eb                                                | 
     100010ec  mov    DWORD PTR [esp+0x2c],eax               | mov    DWORD PTR [esp+0x2c],eax
     100010f0  mov    eax,DWORD PTR [esp+0x18]               | mov    eax,DWORD PTR [esp+0x18]
     100010f4  test   BYTE PTR [eax],0x2                     | test   BYTE PTR [eax],0x2
     100010f7  je     0x10001102                             | je     0x10001102
     100010f9  mov    ecx,DWORD PTR [eax+0xc]                | mov    ecx,DWORD PTR [eax+0xc]
     100010fc  mov    DWORD PTR [esp+0x10],ecx               | mov    DWORD PTR [esp+0x10],ecx
     10001100  jmp    0x10001116                             | jmp    0x10001116
     10001102  push   0x0                                    | push   0x0
     10001104  push   0x10015014                             | push   0x10015014
     10001109  push   0x10015024                             | push   0x10015024
     1000110e  push   0x0                                    | push   0x0
     10001110  call   DWORD PTR ds:0x100161ac                | call   DWORD PTR ds:0x100161ac
     10001116  xor    ecx,ecx                                | xor    ecx,ecx
     10001118  mov    cx,WORD PTR [esi+0x1000f892]           | mov    cx,WORD PTR [esi+0x1000f892]
     1000111f  mov    eax,ecx                                | mov    eax,ecx
     10001121  lea    edx,[ecx+ecx*8]                        | lea    edx,[ecx+ecx*8]
     10001124  lea    ecx,[eax+edx*8]                        | lea    ecx,[eax+edx*8]
     10001127  mov    edx,DWORD PTR [esp+0x10]               | mov    edx,DWORD PTR [esp+0x10]
     1000112b  lea    ecx,[ecx+ecx*4]                        | lea    ecx,[ecx+ecx*4]
     1000112e  sub    ecx,eax                                | sub    ecx,eax
     10001130  mov    eax,DWORD PTR [ecx+ecx*2+0x10007008]   | mov    eax,DWORD PTR [ecx+ecx*2+0x10007008]
     10001137  push   eax                                    | push   eax
     10001138  push   edx                                    | push   edx
     10001139  call   DWORD PTR ds:0x10016150                | call   DWORD PTR ds:0x10016150
     1000113f  mov    DWORD PTR [esp+0x30],eax               | mov    DWORD PTR [esp+0x30],eax
     10001143  mov    eax,DWORD PTR [esp+0x5c]               | mov    eax,DWORD PTR [esp+0x5c]
     10001147  cmp    eax,0x3                                | cmp    eax,0x3
     1000114a  ja     0x10001312                             | ja     0x10001312
     10001150  jmp    DWORD PTR [eax*4+0x10001384]           | jmp    DWORD PTR [eax*4+0x10001384]
     10001157  shl    ebx,0x4                                | shl    ebx,0x4
     1000115a  mov    esi,DWORD PTR [esp+0x4c]               | mov    esi,DWORD PTR [esp+0x4c]
     1000115e  cmp    esi,ebx                                | cmp    esi,ebx
     10001160  jle    0x10001164                             | jle    0x10001164
     10001162  mov    esi,ebx                                | mov    esi,ebx
     10001164  lea    eax,[ebp*8+0x0]                        | lea    eax,[ebp*8+0x0]
     1000116b  mov    edi,DWORD PTR [esp+0x50]               | mov    edi,DWORD PTR [esp+0x50]
     1000116f  cmp    eax,edi                                | cmp    eax,edi
     10001171  jge    0x10001175                             | jge    0x10001175
     10001173  mov    edi,eax                                | mov    edi,eax
     10001175  call   DWORD PTR ds:0x100161b4                | call   DWORD PTR ds:0x100161b4
     1000117b  mov    ecx,DWORD PTR [esp+0x44]               | mov    ecx,DWORD PTR [esp+0x44]
     1000117f  push   0xcc0020                               | push   0xcc0020
     10001184  mov    edx,DWORD PTR [esp+0x14]               | mov    edx,DWORD PTR [esp+0x14]
     10001188  sub    edi,DWORD PTR [esp+0x4c]               | sub    edi,DWORD PTR [esp+0x4c]
     1000118c  inc    edi                                    | inc    edi
     1000118d  mov    DWORD PTR [esp+0x20],eax               | mov    DWORD PTR [esp+0x20],eax
     10001191  mov    eax,DWORD PTR [esp+0x4c]               | mov    eax,DWORD PTR [esp+0x4c]
     10001195  sub    esi,DWORD PTR [esp+0x48]               | sub    esi,DWORD PTR [esp+0x48]
     10001199  push   eax                                    | push   eax
     1000119a  inc    esi                                    | inc    esi
     1000119b  mov    eax,DWORD PTR [esp+0x28]               | mov    eax,DWORD PTR [esp+0x28]
     1000119f  push   ecx                                    | push   ecx
     100011a0  mov    ecx,DWORD PTR [esp+0x48]               | mov    ecx,DWORD PTR [esp+0x48]
     100011a4  push   edx                                    | push   edx
     100011a5  push   edi                                    | push   edi
     100011a6  push   esi                                    | push   esi
     100011a7  push   0x0                                    | push   0x0
     100011a9  push   eax                                    | push   eax
     100011aa  push   ecx                                    | push   ecx
     100011ab  call   DWORD PTR ds:0x10016158                | call   DWORD PTR ds:0x10016158
     100011b1  call   DWORD PTR ds:0x100161b4                | call   DWORD PTR ds:0x100161b4
     100011b7  jmp    0x10001316                             | jmp    0x10001316
     100011bc  mov    eax,ebx                                | mov    eax,ebx
     100011be  mov    esi,DWORD PTR [esp+0x4c]               | mov    esi,DWORD PTR [esp+0x4c]
     100011c2  shl    eax,0x4                                | shl    eax,0x4
     100011c5  cmp    esi,eax                                | cmp    esi,eax
     100011c7  jle    0x100011cb                             | jle    0x100011cb
     100011c9  mov    esi,eax                                | mov    esi,eax
     100011cb  lea    eax,[ebp*8+0x0]                        | lea    eax,[ebp*8+0x0]
     100011d2  mov    edi,DWORD PTR [esp+0x50]               | mov    edi,DWORD PTR [esp+0x50]
     100011d6  cmp    eax,edi                                | cmp    eax,edi
     100011d8  jge    0x100011dc                             | jge    0x100011dc
     100011da  mov    edi,eax                                | mov    edi,eax
     100011dc  call   DWORD PTR ds:0x100161b4                | call   DWORD PTR ds:0x100161b4
     100011e2  mov    DWORD PTR [esp+0x1c],eax               | mov    DWORD PTR [esp+0x1c],eax
     100011e6  mov    eax,DWORD PTR [esp+0x24]               | mov    eax,DWORD PTR [esp+0x24]
     100011ea  add    eax,0xc                                | add    eax,0xc
     100011ed  mov    edx,DWORD PTR [esp+0x28]               | mov    edx,DWORD PTR [esp+0x28]
     100011f1  push   eax                                    | push   eax
     100011f2  mov    eax,DWORD PTR [esp+0x28]               | mov    eax,DWORD PTR [esp+0x28]
     100011f6  mov    ecx,DWORD PTR [eax+0x440]              | mov    ecx,DWORD PTR [eax+0x440]
     100011fc  mov    eax,DWORD PTR [esp+0x4c]               | mov    eax,DWORD PTR [esp+0x4c]
     10001200  push   ecx                                    | push   ecx
     10001201  push   ebp                                    | push   ebp
     10001202  mov    ecx,DWORD PTR [esp+0x50]               | mov    ecx,DWORD PTR [esp+0x50]
     10001206  push   ebx                                    | push   ebx
     10001207  push   edx                                    | push   edx
     10001208  push   edi                                    | push   edi
     10001209  mov    edx,DWORD PTR [esp+0x28]               | mov    edx,DWORD PTR [esp+0x28]
     1000120d  push   esi                                    | push   esi
     1000120e  push   eax                                    | push   eax
     1000120f  push   ecx                                    | push   ecx
     10001210  mov    eax,DWORD PTR [esp+0x50]               | mov    eax,DWORD PTR [esp+0x50]
     10001214  push   edx                                    | push   edx
     10001215  mov    ecx,DWORD PTR [esp+0x48]               | mov    ecx,DWORD PTR [esp+0x48]
     10001219  push   eax                                    | push   eax
     1000121a  mov    edx,DWORD PTR [esp+0x68]               | mov    edx,DWORD PTR [esp+0x68]
     1000121e  push   ecx                                    | push   ecx
     1000121f  push   edx                                    | push   edx
     10001220  call   0x10004af0                             | call   0x10004af0
     10001225  add    esp,0x34                               | add    esp,0x34
     10001228  call   DWORD PTR ds:0x100161b4                | call   DWORD PTR ds:0x100161b4
     1000122e  jmp    0x10001316                             | jmp    0x10001316
     10001233  mov    eax,ebx                                | mov    eax,ebx
     10001235  mov    esi,DWORD PTR [esp+0x4c]               | mov    esi,DWORD PTR [esp+0x4c]
     10001239  shl    eax,0x4                                | shl    eax,0x4
     1000123c  cmp    esi,eax                                | cmp    esi,eax
     1000123e  jle    0x10001242                             | jle    0x10001242
     10001240  mov    esi,eax                                | mov    esi,eax
     10001242  lea    eax,[ebp*8+0x0]                        | lea    eax,[ebp*8+0x0]
     10001249  mov    edi,DWORD PTR [esp+0x50]               | mov    edi,DWORD PTR [esp+0x50]
     1000124d  cmp    eax,edi                                | cmp    eax,edi
     1000124f  jge    0x10001253                             | jge    0x10001253
     10001251  mov    edi,eax                                | mov    edi,eax
     10001253  call   DWORD PTR ds:0x100161b4                | call   DWORD PTR ds:0x100161b4
     10001259  mov    DWORD PTR [esp+0x1c],eax               | mov    DWORD PTR [esp+0x1c],eax
     1000125d  mov    eax,DWORD PTR [esp+0x24]               | mov    eax,DWORD PTR [esp+0x24]
     10001261  add    eax,0xc                                | add    eax,0xc
     10001264  mov    edx,DWORD PTR [esp+0x58]               | mov    edx,DWORD PTR [esp+0x58]
     10001268  push   eax                                    | push   eax
     10001269  mov    eax,DWORD PTR [esp+0x28]               | mov    eax,DWORD PTR [esp+0x28]
     1000126d  mov    ecx,DWORD PTR [eax+0x440]              | mov    ecx,DWORD PTR [eax+0x440]
     10001273  mov    eax,DWORD PTR [esp+0x58]               | mov    eax,DWORD PTR [esp+0x58]
     10001277  push   ecx                                    | push   ecx
     10001278  push   edx                                    | push   edx
     10001279  mov    ecx,DWORD PTR [esp+0x34]               | mov    ecx,DWORD PTR [esp+0x34]
     1000127d  push   eax                                    | push   eax
     1000127e  mov    edx,DWORD PTR [esp+0x58]               | mov    edx,DWORD PTR [esp+0x58]
     10001282  push   ebp                                    | push   ebp
     10001283  mov    eax,DWORD PTR [esp+0x58]               | mov    eax,DWORD PTR [esp+0x58]
     10001287  push   ebx                                    | push   ebx
     10001288  push   ecx                                    | push   ecx
     10001289  push   edi                                    | push   edi
     1000128a  mov    ecx,DWORD PTR [esp+0x30]               | mov    ecx,DWORD PTR [esp+0x30]
     1000128e  push   esi                                    | push   esi
     1000128f  push   edx                                    | push   edx
     10001290  push   eax                                    | push   eax
     10001291  mov    edx,DWORD PTR [esp+0x58]               | mov    edx,DWORD PTR [esp+0x58]
     10001295  push   ecx                                    | push   ecx
     10001296  mov    eax,DWORD PTR [esp+0x50]               | mov    eax,DWORD PTR [esp+0x50]
     1000129a  push   edx                                    | push   edx
     1000129b  mov    ecx,DWORD PTR [esp+0x70]               | mov    ecx,DWORD PTR [esp+0x70]
     1000129f  push   eax                                    | push   eax
     100012a0  push   ecx                                    | push   ecx
     100012a1  call   0x10004bd0                             | call   0x10004bd0
     100012a6  add    esp,0x3c                               | add    esp,0x3c
     100012a9  call   DWORD PTR ds:0x100161b4                | call   DWORD PTR ds:0x100161b4
     100012af  jmp    0x10001316                             | jmp    0x10001316
     100012b1  mov    eax,ebx                                | mov    eax,ebx
     100012b3  mov    esi,DWORD PTR [esp+0x4c]               | mov    esi,DWORD PTR [esp+0x4c]
     100012b7  shl    eax,0x4                                | shl    eax,0x4
     100012ba  cmp    esi,eax                                | cmp    esi,eax
     100012bc  jle    0x100012c0                             | jle    0x100012c0
     100012be  mov    esi,eax                                | mov    esi,eax
     100012c0  lea    eax,[ebp*8+0x0]                        | lea    eax,[ebp*8+0x0]
     100012c7  mov    edi,DWORD PTR [esp+0x50]               | mov    edi,DWORD PTR [esp+0x50]
     100012cb  cmp    eax,edi                                | cmp    eax,edi
     100012cd  jge    0x100012d1                             | jge    0x100012d1
     100012cf  mov    edi,eax                                | mov    edi,eax
     100012d1  call   DWORD PTR ds:0x100161b4                | call   DWORD PTR ds:0x100161b4
     100012d7  mov    ecx,DWORD PTR [esp+0x54]               | mov    ecx,DWORD PTR [esp+0x54]
     100012db  mov    DWORD PTR [esp+0x1c],eax               | mov    DWORD PTR [esp+0x1c],eax
     100012df  mov    eax,DWORD PTR [esp+0x58]               | mov    eax,DWORD PTR [esp+0x58]
     100012e3  mov    edx,DWORD PTR [esp+0x28]               | mov    edx,DWORD PTR [esp+0x28]
     100012e7  push   eax                                    | push   eax
     100012e8  push   ecx                                    | push   ecx
     100012e9  mov    eax,DWORD PTR [esp+0x50]               | mov    eax,DWORD PTR [esp+0x50]
     100012ed  push   ebp                                    | push   ebp
     100012ee  mov    ecx,DWORD PTR [esp+0x50]               | mov    ecx,DWORD PTR [esp+0x50]
     100012f2  push   ebx                                    | push   ebx
     100012f3  push   edx                                    | push   edx
     100012f4  push   edi                                    | push   edi
     100012f5  mov    edx,DWORD PTR [esp+0x28]               | mov    edx,DWORD PTR [esp+0x28]
     100012f9  push   esi                                    | push   esi
     100012fa  push   eax                                    | push   eax
     100012fb  push   ecx                                    | push   ecx
     100012fc  mov    eax,DWORD PTR [esp+0x60]               | mov    eax,DWORD PTR [esp+0x60]
     10001300  push   edx                                    | push   edx
     10001301  push   eax                                    | push   eax
     10001302  call   0x10004d10                             | call   0x10004d10
     10001307  add    esp,0x2c                               | add    esp,0x2c
     1000130a  call   DWORD PTR ds:0x100161b4                | call   DWORD PTR ds:0x100161b4
     10001310  jmp    0x10001316                             | jmp    0x10001316
     10001312  mov    eax,DWORD PTR [esp+0x30]               | mov    eax,DWORD PTR [esp+0x30]
     10001316  sub    eax,DWORD PTR [esp+0x1c]               | sub    eax,DWORD PTR [esp+0x1c]
     1000131a  add    DWORD PTR ds:0x1001500c,eax            | add    DWORD PTR ds:0x1001500c,eax
     10001320  call   DWORD PTR ds:0x10016140                | call   DWORD PTR ds:0x10016140
     10001326  mov    ecx,DWORD PTR [esp+0x18]               | mov    ecx,DWORD PTR [esp+0x18]
     1000132a  test   BYTE PTR [ecx],0x2                     | test   BYTE PTR [ecx],0x2
     1000132d  je     0x10001357                             | je     0x10001357
     1000132f  call   DWORD PTR ds:0x100161b4                | call   DWORD PTR ds:0x100161b4
     10001335  mov    esi,eax                                | mov    esi,eax
     10001337  mov    ecx,DWORD PTR [esp+0x14]               | mov    ecx,DWORD PTR [esp+0x14]
     1000133b  mov    eax,DWORD PTR [esp+0x40]               | mov    eax,DWORD PTR [esp+0x40]
     1000133f  push   eax                                    | push   eax
     10001340  push   ecx                                    | push   ecx
     10001341  call   0x10004d50                             | call   0x10004d50
     10001346  add    esp,0x8                                | add    esp,0x8
     10001349  call   DWORD PTR ds:0x100161b4                | call   DWORD PTR ds:0x100161b4
     1000134f  sub    eax,esi                                | sub    eax,esi
     10001351  add    DWORD PTR ds:0x10015010,eax            | add    DWORD PTR ds:0x10015010,eax
     10001357  mov    eax,DWORD PTR [esp+0x40]               | mov    eax,DWORD PTR [esp+0x40]
     1000135b  mov    ecx,DWORD PTR [esp+0x14]               | mov    ecx,DWORD PTR [esp+0x14]
     1000135f  push   eax                                    | push   eax
     10001360  push   ecx                                    | push   ecx
     10001361  call   0x10003880                             | call   0x10003880
     10001366  mov    ecx,DWORD PTR [esp+0x38]               | mov    ecx,DWORD PTR [esp+0x38]
     1000136a  mov    edx,DWORD PTR [esp+0x18]               | mov    edx,DWORD PTR [esp+0x18]
     1000136e  add    esp,0x8                                | add    esp,0x8
     10001371  push   ecx                                    | push   ecx
     10001372  push   edx                                    | push   edx
     10001373  call   DWORD PTR ds:0x10016150                | call   DWORD PTR ds:0x10016150
     10001379  pop    ebp                                    | pop    ebp
     1000137a  pop    edi                                    | pop    edi
     1000137b  pop    esi                                    | pop    esi
     1000137c  pop    ebx                                    | pop    ebx
     1000137d  add    esp,0x24                               | add    esp,0x24
     10001380  ret    0x28                                   | ret    0x28
     10001383  nop                                           | nop
     10001384  push   edi                                    | push   edi
     10001385  adc    DWORD PTR [eax],eax                    | adc    DWORD PTR [eax],eax
     10001387  adc    BYTE PTR [ecx+edx*1+0x12331000],bh     | adc    BYTE PTR [ecx+edx*1+0x12331000],bh
     1000138e  add    BYTE PTR [eax],dl                      | add    BYTE PTR [eax],dl
     10001390  mov    cl,0x12                                | mov    cl,0x12
     10001392  add    BYTE PTR [eax],dl                      | add    BYTE PTR [eax],dl
     10001394  add    eax,0x0                                | add    eax,0x0
     10001399  lea    esp,[esp+0x0]                          | lea    esp,[esp+0x0]
```

## What HANDOFF.md already records (most recent last)
- - Session 31: Renderer_Render 7 -> 5 (applied): setup order now c, dx = 0, ... (greedy over the 6 setup lines, `s30_mdib/rr_greedy.log`); RENDER 74/77 intact; make OK. 300 random decl orders (old body) and CopyChanged 300 decl orders: no gain. levelsel joint padding (600): current is best. Backup `build/scratch/s30_mdib/render.c.before3`.
- - Session 31: Renderer_Render further greedy blocks (changed/across/down order, dx/dy if-body orders, r/s order): stays 5; 300 decl orders on the new body: stays 5. CopyChanged without bits/bmi parameters: wrong - the original Renderer_Render pushes them (function shrinks 896->864 and 30 functions shift), so the parameters are real. Not applied.
- - Optimisation-mode probe (`tools/msvc/optmode.py`, all modes incl. size) on Renderer_Render, CopyChanged, StretchChanged: every mode worse or equal; all three are /Ox functions. `#pragma optimize("l"/"p")` do not exist in VC 2.2 (no effect).
- - **Renderer_Render (5):** remaining diff = across computed after down; the original order looks like `c; changed; across; down; dx = 0; dy = 0` (our pre-session order, which had the zero-register swap instead). Running: that order x pads 0-128.
- - RENDER deep dive, continued: CopyChanged with the original statement order: decl-order permutations of the 4 loop vars x pads 0-40 (2952) best 26; declaration hill-climb (new tool `tools/msvc/search/declclimb.py`) 22 at pad 27; split `row = changed; ... row += across * cy0;` in every placement + decl-initialiser form x pads 0-40 (1066) best 24. The `cy0; row; cx1; cx0; cy1` order + 27 pads (15, loop fully matching) remains the best scratch result. Renderer_Render: hill-climb from the plain order stays 7 (production 5 kept). StretchChanged: sx/w/h volatile (5), address-taken sy (21) worse; statement-order greedy stays 3.
- - CopyChanged register study (`build/scratch/s31_render/cc_regs.py`/`.log`, all 120 statement orders at pad 0): `row` gets the original's edi ONLY when the first two statements are `cy0 = ...; row = ...;`; every order starting with cx0 or cy1 gives row=esi. The original computes cx0 first, so testing combined forms `row = changed + across * (cy0 = top >> 3);` in all positions x pads 0-40 (`cc8.txt`). Declaration climbs from the best (15, pad 27) and cross searches confirm 15 is the floor of the separate-statement forms. Renderer_Render declaration climb from production: stays 5. StretchChanged: `for` loops give the same counter code as the original (counter in cy0's slot) but keep the early 
-   - Common pattern in all three misses: the original puts a short-lived expression temp in **eax** (StretchChanged and CopyChanged: the `cy0 * across` row product; Renderer_Render: `left & 15`, with the dx/dy zero in ecx) where our VC 2.2 build picks ecx/edx. Ruled out as causes: optimisation pragmas (all modes), build flags (/Zd /Z7 /Zi /Gd /J same; /Gf /Gy /Ge /Gh worse), the VC 2.0 compiler (same results, 34/77 overall), unsigned/DWORD/long types, struct/array/address-taken/global/static forms of sy, extra volatiles, local names (no name-hash effect), unused leftover declarations.
-   - Renderer_Render: `c; dx = 0; changed; across; down; dy = 0;` gives every register right but schedules the canvas-address lea/sub before `across` (6); production order (5) kept.
- - Flag sweep 2 (whole RENDER, `CFLAGS_EXTRA`): /Ol /On /Oo /Oo- /Oc /Oe /Oz /Ob1 /Og /Qifist /Gi /Gx /Gt /Op- identical to /Ox (74/77); /Oa /Ow /Oxa 65-66 (Renderer_Render 12); /Op 66; /O2 /Ox2 60. No flag changes the three misses for the better (`build/scratch/s33_sc/flags2.log`).
- - Session 33, Renderer_Render: all 720 orders of the 6 setup statements (c, dx, changed, down, across, dy) at the current padding: best 5 = production (`build/scratch/s33_rr/perm6.log`). Not a statement-order problem at this padding.
- - **Re-verified 2026-11-02 (fresh `build.py --funcs` per module, `build/scratch/s33_bp/verify2.log`, `verify3.log`, `verify_RENDER.log`):** EXE 324/334, PROCESS 198/206, RENDER 74/77 = **675/696**, which agrees with the header. StartGame and SaveLayout now MATCH (another session's work; this supersedes the 'SaveLayout not applied' note above). SoundInit MATCH. Current misses: EXE (10): ReadIniSettings 3, Bitmap::BuildRows 4, ApplyOptions 7, MapView::DrawLemmings 7, DockWnd::Drag 17, ScrollTo 23, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, SetViewScale 46. PROCESS (8): BuildCollision 2, Builder::Process 3, Map::FillObjects 4, WaveTestVolume 13, Map::AddTrapType 17, Map::AddObjectTy
- - Session 33, inline/padding hunt (no new match): `Message` (lemming.h, a plain inline function, not a class member) dummy locals 0-8 x padding for Miner (pad_top 0-40) and Basher (pad_basherprocess 0-40): no effect (hook removed again, lemming.h unchanged). End-of-file padding (file-wide symbol count) does not move non-inline functions (Builder, WaveTestVolume, Miner, Basher, AddObjectType all unchanged), so the count effect is specific to class-member inlines. pad_top 0-64 for AddObjectType/AddTrapType: no gain. BuildCollision with all declarations at the top + declclimb: 2. Renderer_Render: 8 best setup orders x pads 0-64 and 5 canvas-pointer spellings: 5.
- - Session 33 real-name climbs (splitclimb.py, pads 0-3): StretchChanged 3 (same as production), CopyChanged production 17, natural-order 24, cc40 order 19, Renderer_Render split body 13 (production 5). ApplyOptions late-zero body, pads 0-40: 22 (swap). No match. The earlier variants.py results for RENDER were not wrong-context artefacts.
-   - `RENDER.DLL`: **75/77** (+1 match; only 2 functions remain: `Renderer_Render` 5, `CopyChanged` 13).

## Hints
- Differences are almost always a register/ordering choice by the compiler. Levers that have worked elsewhere: `extern int pad_xxxN;` symbol padding, declaration order and locals, block-scoped temporaries `{ T t = expr; v = t; }`, deriving a field from a just-stored field, `#pragma optimize("t", off)` plus a naked `_emit` padding stub for size-mode functions.
- Write new reusable scripts under `tools/`, record negatives in HANDOFF.md.

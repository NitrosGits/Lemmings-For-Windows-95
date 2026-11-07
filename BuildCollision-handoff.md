# BuildCollision hand-off

> Generated 2026-11-01. Project: `/Users/daniel/Lemmings-For-Windows-95` (not a git repo). Module: PROCESS.DLL, function `BuildCollision` at **0x100051f0** (static, in `src/process/map.cpp`), **32 instructions differ** (the whole function: 400 bytes incl. padding). Everything else in PROCESS matches except 7 other functions; PROCESS is 198/206.

> **UPDATE (same day, after the other agent's suggestion): BuildCollision is now 2 instructions off (was 32) and APPLIED to `src/process/map.cpp`.** The fix was idea 1: write the LAST bit of the scale-1 arm as a block-scoped temporary, `{ BYTE b = *s ? 0x01 : 0; *d |= b; }` (a sweep of all 255 plain/block-temp combinations for the 8 bits found that exactly one block-temp at bit 0x01, 0x04, 0x10 or 0x40 gives 2; the all-plain form gives 32). PROCESS link: 198/206, only BuildCollision changed; map.cpp re-padded (`pad_mapmap0`/`pad_mapsetobjectat*` via padplan, `pad_mapfillobjects0..5` restored). **Remaining 2-instruction diff, the whole function prologue:** original `mov ebp,[esp+0x1c]; mov edi,[esp+0x20]; push 0x10011548(string)`, ours `mov ebp,[esp+0x20]; push <string>; mov edi,[esp+0x24]`, i.e. the original loads BOTH register parameters (w then h) before pushing the string argument of `VSMEM_Allocate`, ours loads `h` only after the push (only `w` is hoisted to the entry). Tried on top of the fix, all still 2: `h * ((w+7)/8)`, `>> 3`, cached `rows`/`cols`/`size`/`bytes` locals, block-scoped size temp, unused locals using h/w, 8 declaration arrangements, count-down row loops, pads 0-7. Specs: `build/scratch/s32/bc4..bc11_spec.py`. Original text below this note describes the 32-instruction state.

## Goal

Make `BuildCollision` in `src/process/map.cpp` compile under Visual C++ 2.2 (`/Ox`) to bytes identical to the original `orig/WINLEMM/PROCESS.DLL`, without breaking the other 22 functions in map.cpp (19/23 match now) and without breaking the MinGW build (`make`).

## What the function does

Builds a 1-bit-per-pixel collision map from an 8-bit bitmap: one output byte per 8 pixels, bit set if the pixel is non-zero. At scale 2 (`g_scale == 2`, big graphics) every other pixel and every other row is used (hence the `s++` skips and the `s += w * 2` row skip). Output is allocated with `VSMEM_Allocate((w + 7) / 8 * h, "PROCESS(map.cpp): Collision map array")`.

## Current source (what does not match)

```cpp
/* 0x100051f0: a bit for each pixel that isn't 0 (every other pixel and row at scale 2) */
static BYTE *BuildCollision(BYTE *pixels, int w, int h)
{
    BYTE *bits = (BYTE *)VSMEM_Allocate((w + 7) / 8 * h, "PROCESS(map.cpp): Collision map array");
    BYTE *d = bits;
    BYTE *s = pixels;
    int x, y;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x += 8, s++, d++) {
            if (g_scale == 2) {
                *d = 0;
                *d |= *s++ ? 0x80 : 0;
                s++;
                *d |= *s++ ? 0x40 : 0;
                s++;
                *d |= *s++ ? 0x20 : 0;
                s++;
                *d |= *s++ ? 0x10 : 0;
                s++;
                *d |= *s++ ? 0x08 : 0;
                s++;
                *d |= *s++ ? 0x04 : 0;
                s++;
                *d |= *s++ ? 0x02 : 0;
                s++;
                *d |= *s++ ? 0x01 : 0;
            } else {
                *d = 0;
                *d |= *s++ ? 0x80 : 0;
                *d |= *s++ ? 0x40 : 0;
                *d |= *s++ ? 0x20 : 0;
                *d |= *s++ ? 0x10 : 0;
                *d |= *s++ ? 0x08 : 0;
                *d |= *s++ ? 0x04 : 0;
                *d |= *s++ ? 0x02 : 0;
                *d |= *s ? 0x01 : 0;
            }
        }
        if (g_scale == 2)
            s += w * 2;
    }
    return bits;
}
```

Context: `g_scale` is an `int` global (0x100112d8); `VSMEM_Allocate(size, name)` is a stdcall import; `BYTE` is `unsigned char`. The function is called from `Map::SetMap` (0x100056b0). The file has pad externs before some functions (`pad_*`: VC 2.x code depends on how many symbols were declared before a function; see `docs/DECOMP.md`); BuildCollision currently has none.

## What is off (the whole story)

The scale-2 arm (the fall-through arm) already matches byte for byte. The diff starts in the prologue and in the scale-1 (`else`) arm; nearly all of it follows from ONE register-allocation difference:

* In the **original scale-1 arm** the byte temporaries alternate cleanly `bl, dl, bl, dl, ...` (first temp is `bl`, with `mov bl,0 / mov dl,0` pre-zeroing, then `adc bl,0xff; and bl,0x80; mov [ecx],bl; ... adc dl,0xff; and dl,0x40; or dl,bl; mov bl,0 ...`; the last bit is `sbb dl,dl / inc dl / or dl,bl`).
* In **ours** the scale-1 arm starts with `dl` (`mov dl,0 / mov bl,0`), and at the third bit it reuses `bl` while the second temp is still live, so it spills the byte to `[esp+0x10]` (`mov BYTE PTR [esp+0x10],bl` ... `or bl,BYTE PTR [esp+0x10]`).
* That spill is also the reason ours has `sub esp,0x8` / `add esp,0x8` (two stack slots: the row counter and the spill byte) where the original has `sub esp,0x4` / `add esp,0x4` (only the row counter at `[esp+0x10]`). It also shifts the argument loads (`mov ebp,[esp+0x20]` vs the original's `mov ebp,[esp+0x1c]`) and every jump offset after it.
* In **both** compiles the scale-2 arm starts with `dl`, and the original's scale-1 arm starts with `bl`. Both arms hoist `*d = 0` into one `mov BYTE PTR [ecx],0x0` before the `jne`.

So the question for the next agent: what source form makes VC 2.2 start the scale-1 arm's temps in `bl` and allocate them cleanly with no spill?

## Original assembly (`orig/WINLEMM/PROCESS.DLL`, llvm-objdump syntax with bytes; 0x100051f0-0x1000537f incl. padding)

```
100051f0: 83 ec 04                     	sub	esp, 0x4
100051f3: 53                           	push	ebx
100051f4: 56                           	push	esi
100051f5: 57                           	push	edi
100051f6: 55                           	push	ebp
100051f7: 8b 6c 24 1c                  	mov	ebp, dword ptr [esp + 0x1c]
100051fb: 8b 7c 24 20                  	mov	edi, dword ptr [esp + 0x20]
100051ff: 68 48 15 01 10               	push	0x10011548
10005204: 8d 45 07                     	lea	eax, [ebp + 0x7]
10005207: 99                           	cdq
10005208: 83 e2 07                     	and	edx, 0x7
1000520b: 03 c2                        	add	eax, edx
1000520d: c1 f8 03                     	sar	eax, 0x3
10005210: 0f af c7                     	imul	eax, edi
10005213: 50                           	push	eax
10005214: e8 1f 3b 00 00               	call	0x10008d38 <?WaveTestVolume@@YGXH@Z+0x27d8>
10005219: 8b c8                        	mov	ecx, eax
1000521b: 8b 74 24 18                  	mov	esi, dword ptr [esp + 0x18]
1000521f: 85 ff                        	test	edi, edi
10005221: 0f 8e 3d 01 00 00            	jle	0x10005364 <?CheckScript@@YGXH@Z+0x1264>
10005227: 89 7c 24 10                  	mov	dword ptr [esp + 0x10], edi
1000522b: 85 ed                        	test	ebp, ebp
1000522d: 0f 8e 1b 01 00 00            	jle	0x1000534e <?CheckScript@@YGXH@Z+0x124e>
10005233: 8d 7d 07                     	lea	edi, [ebp + 0x7]
10005236: c1 ef 03                     	shr	edi, 0x3
10005239: 83 3d d8 12 01 10 02         	cmp	dword ptr [0x100112d8], 0x2
10005240: c6 01 00                     	mov	byte ptr [ecx], 0x0
10005243: 0f 85 83 00 00 00            	jne	0x100052cc <?CheckScript@@YGXH@Z+0x11cc>
10005249: b2 00                        	mov	dl, 0x0
1000524b: b3 00                        	mov	bl, 0x0
1000524d: 80 3e 01                     	cmp	byte ptr [esi], 0x1
10005250: 80 d2 ff                     	adc	dl, -0x1
10005253: 46                           	inc	esi
10005254: 80 e2 80                     	and	dl, -0x80
10005257: 46                           	inc	esi
10005258: 88 11                        	mov	byte ptr [ecx], dl
1000525a: 80 3e 01                     	cmp	byte ptr [esi], 0x1
1000525d: 80 d3 ff                     	adc	bl, -0x1
10005260: 46                           	inc	esi
10005261: 80 e3 40                     	and	bl, 0x40
10005264: 46                           	inc	esi
10005265: 0a da                        	or	bl, dl
10005267: b2 00                        	mov	dl, 0x0
10005269: 88 19                        	mov	byte ptr [ecx], bl
1000526b: 80 3e 01                     	cmp	byte ptr [esi], 0x1
1000526e: 80 d2 ff                     	adc	dl, -0x1
10005271: 46                           	inc	esi
10005272: 80 e2 20                     	and	dl, 0x20
10005275: 46                           	inc	esi
10005276: 0a d3                        	or	dl, bl
10005278: b3 00                        	mov	bl, 0x0
1000527a: 88 11                        	mov	byte ptr [ecx], dl
1000527c: 80 3e 01                     	cmp	byte ptr [esi], 0x1
1000527f: 80 d3 ff                     	adc	bl, -0x1
10005282: 46                           	inc	esi
10005283: 80 e3 10                     	and	bl, 0x10
10005286: 46                           	inc	esi
10005287: 0a da                        	or	bl, dl
10005289: b2 00                        	mov	dl, 0x0
1000528b: 88 19                        	mov	byte ptr [ecx], bl
1000528d: 80 3e 01                     	cmp	byte ptr [esi], 0x1
10005290: 80 d2 ff                     	adc	dl, -0x1
10005293: 46                           	inc	esi
10005294: 80 e2 08                     	and	dl, 0x8
10005297: 46                           	inc	esi
10005298: 0a d3                        	or	dl, bl
1000529a: b3 00                        	mov	bl, 0x0
1000529c: 88 11                        	mov	byte ptr [ecx], dl
1000529e: 80 3e 01                     	cmp	byte ptr [esi], 0x1
100052a1: 80 d3 ff                     	adc	bl, -0x1
100052a4: 46                           	inc	esi
100052a5: 80 e3 04                     	and	bl, 0x4
100052a8: 46                           	inc	esi
100052a9: 0a da                        	or	bl, dl
100052ab: b2 00                        	mov	dl, 0x0
100052ad: 88 19                        	mov	byte ptr [ecx], bl
100052af: 80 3e 01                     	cmp	byte ptr [esi], 0x1
100052b2: 80 d2 ff                     	adc	dl, -0x1
100052b5: 46                           	inc	esi
100052b6: 80 e2 02                     	and	dl, 0x2
100052b9: 46                           	inc	esi
100052ba: 0a d3                        	or	dl, bl
100052bc: 88 11                        	mov	byte ptr [ecx], dl
100052be: 80 3e 01                     	cmp	byte ptr [esi], 0x1
100052c1: 1a db                        	sbb	bl, bl
100052c3: fe c3                        	inc	bl
100052c5: 0a da                        	or	bl, dl
100052c7: 88 19                        	mov	byte ptr [ecx], bl
100052c9: 46                           	inc	esi
100052ca: eb 79                        	jmp	0x10005345 <?CheckScript@@YGXH@Z+0x1245>
100052cc: b3 00                        	mov	bl, 0x0
100052ce: b2 00                        	mov	dl, 0x0
100052d0: 80 3e 01                     	cmp	byte ptr [esi], 0x1
100052d3: 80 d3 ff                     	adc	bl, -0x1
100052d6: 46                           	inc	esi
100052d7: 80 e3 80                     	and	bl, -0x80
100052da: 88 19                        	mov	byte ptr [ecx], bl
100052dc: 80 3e 01                     	cmp	byte ptr [esi], 0x1
100052df: 80 d2 ff                     	adc	dl, -0x1
100052e2: 46                           	inc	esi
100052e3: 80 e2 40                     	and	dl, 0x40
100052e6: 0a d3                        	or	dl, bl
100052e8: b3 00                        	mov	bl, 0x0
100052ea: 88 11                        	mov	byte ptr [ecx], dl
100052ec: 80 3e 01                     	cmp	byte ptr [esi], 0x1
100052ef: 80 d3 ff                     	adc	bl, -0x1
100052f2: 46                           	inc	esi
100052f3: 80 e3 20                     	and	bl, 0x20
100052f6: 0a da                        	or	bl, dl
100052f8: b2 00                        	mov	dl, 0x0
100052fa: 88 19                        	mov	byte ptr [ecx], bl
100052fc: 80 3e 01                     	cmp	byte ptr [esi], 0x1
100052ff: 80 d2 ff                     	adc	dl, -0x1
10005302: 46                           	inc	esi
10005303: 80 e2 10                     	and	dl, 0x10
10005306: 0a d3                        	or	dl, bl
10005308: b3 00                        	mov	bl, 0x0
1000530a: 88 11                        	mov	byte ptr [ecx], dl
1000530c: 80 3e 01                     	cmp	byte ptr [esi], 0x1
1000530f: 80 d3 ff                     	adc	bl, -0x1
10005312: 46                           	inc	esi
10005313: 80 e3 08                     	and	bl, 0x8
10005316: 0a da                        	or	bl, dl
10005318: b2 00                        	mov	dl, 0x0
1000531a: 88 19                        	mov	byte ptr [ecx], bl
1000531c: 80 3e 01                     	cmp	byte ptr [esi], 0x1
1000531f: 80 d2 ff                     	adc	dl, -0x1
10005322: 46                           	inc	esi
10005323: 80 e2 04                     	and	dl, 0x4
10005326: 0a d3                        	or	dl, bl
10005328: b3 00                        	mov	bl, 0x0
1000532a: 88 11                        	mov	byte ptr [ecx], dl
1000532c: 80 3e 01                     	cmp	byte ptr [esi], 0x1
1000532f: 80 d3 ff                     	adc	bl, -0x1
10005332: 46                           	inc	esi
10005333: 80 e3 02                     	and	bl, 0x2
10005336: 0a da                        	or	bl, dl
10005338: 88 19                        	mov	byte ptr [ecx], bl
1000533a: 80 3e 01                     	cmp	byte ptr [esi], 0x1
1000533d: 1a d2                        	sbb	dl, dl
1000533f: fe c2                        	inc	dl
10005341: 0a d3                        	or	dl, bl
10005343: 88 11                        	mov	byte ptr [ecx], dl
10005345: 46                           	inc	esi
10005346: 41                           	inc	ecx
10005347: 4f                           	dec	edi
10005348: 0f 85 eb fe ff ff            	jne	0x10005239 <?CheckScript@@YGXH@Z+0x1139>
1000534e: 83 3d d8 12 01 10 02         	cmp	dword ptr [0x100112d8], 0x2
10005355: 75 03                        	jne	0x1000535a <?CheckScript@@YGXH@Z+0x125a>
10005357: 8d 34 6e                     	lea	esi, [esi + 2*ebp]
1000535a: ff 4c 24 10                  	dec	dword ptr [esp + 0x10]
1000535e: 0f 85 c7 fe ff ff            	jne	0x1000522b <?CheckScript@@YGXH@Z+0x112b>
10005364: 5d                           	pop	ebp
10005365: 5f                           	pop	edi
10005366: 5e                           	pop	esi
10005367: 5b                           	pop	ebx
10005368: 83 c4 04                     	add	esp, 0x4
1000536b: c3                           	ret
1000536c: 8d 64 24 00                  	lea	esp, [esp]
10005370: 53                           	push	ebx
10005371: 56                           	push	esi
10005372: 57                           	push	edi
10005373: 8b f1                        	mov	esi, ecx
10005375: 8b 7c 24 14                  	mov	edi, dword ptr [esp + 0x14]
10005379: 55                           	push	ebp
1000537a: 8b 6c 24 1c                  	mov	ebp, dword ptr [esp + 0x1c]
1000537e: 8b 44 24 14                  	mov	eax, dword ptr [esp + 0x14]
```

## Side by side (ours | original), from `tools/msvc/try.sh PROCESS.DLL src/process/map.cpp -v -f BuildCollision`

`*` marks differing instructions. Left = ours, right = original (the first `push 0x0` vs `push 0x10011548` is only the string-literal relocation, not a real difference).

```
* 100051f0  sub    esp,0x8                             | sub    esp,0x4
  100051f3  push   ebx                                 | push   ebx
  100051f4  push   esi                                 | push   esi
  100051f5  push   edi                                 | push   edi
  100051f6  push   ebp                                 | push   ebp
* 100051f7  mov    ebp,DWORD PTR [esp+0x20]            | mov    ebp,DWORD PTR [esp+0x1c]
* 100051fb  push   0x0                                 | mov    edi,DWORD PTR [esp+0x20]
* 10005200  mov    edi,DWORD PTR [esp+0x28]            | push   0x10011548
  10005204  lea    eax,[ebp+0x7]                       | lea    eax,[ebp+0x7]
  10005207  cdq                                        | cdq
  10005208  and    edx,0x7                             | and    edx,0x7
  1000520b  add    eax,edx                             | add    eax,edx
  1000520d  sar    eax,0x3                             | sar    eax,0x3
  10005210  imul   eax,edi                             | imul   eax,edi
  10005213  push   eax                                 | push   eax
  10005214  call   0x10008d38                          | call   0x10008d38
  10005219  mov    ecx,eax                             | mov    ecx,eax
* 1000521b  mov    esi,DWORD PTR [esp+0x1c]            | mov    esi,DWORD PTR [esp+0x18]
  1000521f  test   edi,edi                             | test   edi,edi
* 10005221  jle    0x1000536a                          | jle    0x10005364
* 10005227  mov    DWORD PTR [esp+0x14],edi            | mov    DWORD PTR [esp+0x10],edi
  1000522b  test   ebp,ebp                             | test   ebp,ebp
* 1000522d  jle    0x10005354                          | jle    0x1000534e
  10005233  lea    edi,[ebp+0x7]                       | lea    edi,[ebp+0x7]
  10005236  shr    edi,0x3                             | shr    edi,0x3
  10005239  cmp    DWORD PTR ds:0x100112d8,0x2         | cmp    DWORD PTR ds:0x100112d8,0x2
  10005240  mov    BYTE PTR [ecx],0x0                  | mov    BYTE PTR [ecx],0x0
  10005243  jne    0x100052cc                          | jne    0x100052cc
  10005249  mov    dl,0x0                              | mov    dl,0x0
  1000524b  mov    bl,0x0                              | mov    bl,0x0
  1000524d  cmp    BYTE PTR [esi],0x1                  | cmp    BYTE PTR [esi],0x1
  10005250  adc    dl,0xff                             | adc    dl,0xff
  10005253  inc    esi                                 | inc    esi
  10005254  and    dl,0x80                             | and    dl,0x80
  10005257  inc    esi                                 | inc    esi
  10005258  mov    BYTE PTR [ecx],dl                   | mov    BYTE PTR [ecx],dl
  1000525a  cmp    BYTE PTR [esi],0x1                  | cmp    BYTE PTR [esi],0x1
  1000525d  adc    bl,0xff                             | adc    bl,0xff
  10005260  inc    esi                                 | inc    esi
  10005261  and    bl,0x40                             | and    bl,0x40
  10005264  inc    esi                                 | inc    esi
  10005265  or     bl,dl                               | or     bl,dl
  10005267  mov    dl,0x0                              | mov    dl,0x0
  10005269  mov    BYTE PTR [ecx],bl                   | mov    BYTE PTR [ecx],bl
  1000526b  cmp    BYTE PTR [esi],0x1                  | cmp    BYTE PTR [esi],0x1
  1000526e  adc    dl,0xff                             | adc    dl,0xff
  10005271  inc    esi                                 | inc    esi
  10005272  and    dl,0x20                             | and    dl,0x20
  10005275  inc    esi                                 | inc    esi
  10005276  or     dl,bl                               | or     dl,bl
  10005278  mov    bl,0x0                              | mov    bl,0x0
  1000527a  mov    BYTE PTR [ecx],dl                   | mov    BYTE PTR [ecx],dl
  1000527c  cmp    BYTE PTR [esi],0x1                  | cmp    BYTE PTR [esi],0x1
  1000527f  adc    bl,0xff                             | adc    bl,0xff
  10005282  inc    esi                                 | inc    esi
  10005283  and    bl,0x10                             | and    bl,0x10
  10005286  inc    esi                                 | inc    esi
  10005287  or     bl,dl                               | or     bl,dl
  10005289  mov    dl,0x0                              | mov    dl,0x0
  1000528b  mov    BYTE PTR [ecx],bl                   | mov    BYTE PTR [ecx],bl
  1000528d  cmp    BYTE PTR [esi],0x1                  | cmp    BYTE PTR [esi],0x1
  10005290  adc    dl,0xff                             | adc    dl,0xff
  10005293  inc    esi                                 | inc    esi
  10005294  and    dl,0x8                              | and    dl,0x8
  10005297  inc    esi                                 | inc    esi
  10005298  or     dl,bl                               | or     dl,bl
  1000529a  mov    bl,0x0                              | mov    bl,0x0
  1000529c  mov    BYTE PTR [ecx],dl                   | mov    BYTE PTR [ecx],dl
  1000529e  cmp    BYTE PTR [esi],0x1                  | cmp    BYTE PTR [esi],0x1
  100052a1  adc    bl,0xff                             | adc    bl,0xff
  100052a4  inc    esi                                 | inc    esi
  100052a5  and    bl,0x4                              | and    bl,0x4
  100052a8  inc    esi                                 | inc    esi
  100052a9  or     bl,dl                               | or     bl,dl
  100052ab  mov    dl,0x0                              | mov    dl,0x0
  100052ad  mov    BYTE PTR [ecx],bl                   | mov    BYTE PTR [ecx],bl
  100052af  cmp    BYTE PTR [esi],0x1                  | cmp    BYTE PTR [esi],0x1
  100052b2  adc    dl,0xff                             | adc    dl,0xff
  100052b5  inc    esi                                 | inc    esi
  100052b6  and    dl,0x2                              | and    dl,0x2
  100052b9  inc    esi                                 | inc    esi
  100052ba  or     dl,bl                               | or     dl,bl
  100052bc  mov    BYTE PTR [ecx],dl                   | mov    BYTE PTR [ecx],dl
  100052be  cmp    BYTE PTR [esi],0x1                  | cmp    BYTE PTR [esi],0x1
  100052c1  sbb    bl,bl                               | sbb    bl,bl
  100052c3  inc    bl                                  | inc    bl
  100052c5  or     bl,dl                               | or     bl,dl
  100052c7  mov    BYTE PTR [ecx],bl                   | mov    BYTE PTR [ecx],bl
  100052c9  inc    esi                                 | inc    esi
* 100052ca  jmp    0x1000534b                          | jmp    0x10005345
* 100052cc  mov    dl,0x0                              | mov    bl,0x0
* 100052ce  mov    bl,0x0                              | mov    dl,0x0
  100052d0  cmp    BYTE PTR [esi],0x1                  | cmp    BYTE PTR [esi],0x1
* 100052d3  adc    dl,0xff                             | adc    bl,0xff
  100052d6  inc    esi                                 | inc    esi
* 100052d7  and    dl,0x80                             | and    bl,0x80
* 100052da  mov    BYTE PTR [ecx],dl                   | mov    BYTE PTR [ecx],bl
  100052dc  cmp    BYTE PTR [esi],0x1                  | cmp    BYTE PTR [esi],0x1
* 100052df  adc    bl,0xff                             | adc    dl,0xff
  100052e2  inc    esi                                 | inc    esi
* 100052e3  and    bl,0x40                             | and    dl,0x40
* 100052e6  or     bl,dl                               | or     dl,bl
* 100052e8  mov    dl,0x0                              | mov    bl,0x0
* 100052ea  mov    BYTE PTR [esp+0x10],bl              | mov    BYTE PTR [ecx],dl
* 100052ee  mov    BYTE PTR [ecx],bl                   | cmp    BYTE PTR [esi],0x1
* 100052f0  mov    bl,0x0                              | adc    bl,0xff
* 100052f2  cmp    BYTE PTR [esi],0x1                  | inc    esi
* 100052f5  adc    bl,0xff                             | and    bl,0x20
* 100052f8  inc    esi                                 | or     bl,dl
* 100052f9  and    bl,0x20                             | mov    dl,0x0
* 100052fc  or     bl,BYTE PTR [esp+0x10]              | mov    BYTE PTR [ecx],bl
* 10005300  mov    BYTE PTR [ecx],bl                   | cmp    BYTE PTR [esi],0x1
* 10005302  cmp    BYTE PTR [esi],0x1                  | adc    dl,0xff
* 10005305  adc    dl,0xff                             | inc    esi
* 10005308  inc    esi                                 | and    dl,0x10
* 10005309  and    dl,0x10                             | or     dl,bl
* 1000530c  or     dl,bl                               | mov    bl,0x0
* 1000530e  mov    bl,0x0                              | mov    BYTE PTR [ecx],dl
* 10005310  mov    BYTE PTR [ecx],dl                   | cmp    BYTE PTR [esi],0x1
* 10005312  cmp    BYTE PTR [esi],0x1                  | adc    bl,0xff
* 10005315  adc    bl,0xff                             | inc    esi
* 10005318  inc    esi                                 | and    bl,0x8
* 10005319  and    bl,0x8                              | or     bl,dl
* 1000531c  or     bl,dl                               | mov    dl,0x0
* 1000531e  mov    dl,0x0                              | mov    BYTE PTR [ecx],bl
* 10005320  mov    BYTE PTR [ecx],bl                   | cmp    BYTE PTR [esi],0x1
* 10005322  cmp    BYTE PTR [esi],0x1                  | adc    dl,0xff
* 10005325  adc    dl,0xff                             | inc    esi
* 10005328  inc    esi                                 | and    dl,0x4
* 10005329  and    dl,0x4                              | or     dl,bl
* 1000532c  or     dl,bl                               | mov    bl,0x0
* 1000532e  mov    bl,0x0                              | mov    BYTE PTR [ecx],dl
* 10005330  mov    BYTE PTR [ecx],dl                   | cmp    BYTE PTR [esi],0x1
* 10005332  cmp    BYTE PTR [esi],0x1                  | adc    bl,0xff
* 10005335  adc    bl,0xff                             | inc    esi
* 10005338  inc    esi                                 | and    bl,0x2
* 10005339  and    bl,0x2                              | or     bl,dl
* 1000533c  or     bl,dl                               | mov    BYTE PTR [ecx],bl
* 1000533e  mov    BYTE PTR [ecx],bl                   | cmp    BYTE PTR [esi],0x1
* 10005340  cmp    BYTE PTR [esi],0x1                  | sbb    dl,dl
* 10005343  sbb    dl,dl                               | inc    dl
* 10005345  inc    dl                                  | or     dl,bl
* 10005347  or     dl,bl                               | mov    BYTE PTR [ecx],dl
* 10005349  mov    BYTE PTR [ecx],dl                   | inc    esi
* 1000534b  inc    esi                                 | inc    ecx
* 1000534c  inc    ecx                                 | dec    edi
* 1000534d  dec    edi                                 | jne    0x10005239
* 1000534e  jne    0x10005239                          | cmp    DWORD PTR ds:0x100112d8,0x2
* 10005354  cmp    DWORD PTR ds:0x100112d8,0x2         | jne    0x1000535a
* 1000535b  jne    0x10005360                          | lea    esi,[esi+ebp*2]
* 1000535d  lea    esi,[esi+ebp*2]                     | dec    DWORD PTR [esp+0x10]
* 10005360  dec    DWORD PTR [esp+0x14]                | jne    0x1000522b
* 10005364  jne    0x1000522b                          | pop    ebp
* 1000536a  pop    ebp                                 | pop    edi
* 1000536b  pop    edi                                 | pop    esi
* 1000536c  pop    esi                                 | pop    ebx
* 1000536d  pop    ebx                                 | add    esp,0x4
* 1000536e  add    esp,0x8                             | ret
* 10005371  ret                                        | lea    esp,[esp+0x0]
```

## How to reproduce and iterate

```bash
cd /Users/daniel/Lemmings-For-Windows-95
zsh tools/msvc/try.sh PROCESS.DLL src/process/map.cpp -v -f BuildCollision   # ours (left) vs original (right)
zsh tools/msvc/try.sh PROCESS.DLL src/process/map.cpp                        # whole file: map.cpp must stay at least 19/23
python3 tools/msvc/shapes.py PROCESS.DLL src/process/map.cpp 0x100051f0 SPEC.py --pads 0,1,2,3 --out build/scratch/DIR --jobs 8   # SLOTS=[(orig_text,[alt,...])] search
python3 tools/msvc/padplan.py PROCESS.DLL src/process/map.cpp --max 64 --jobs 8 --start 0x10005370 --write   # re-pad later functions after a change
python3 tools/msvc/build.py --funcs PROCESS > LOG   # full link; diff its ": DIFF" list against build/scratch/s32/si/process_fo.log
```

`try.sh` compiles with `tools/msvc/cl.sh` (VC++ 2.2 under Wine, `/Ox`); other flags can be tried with `MSVCFLAGS="/Ox /Os" zsh tools/msvc/try.sh ...`. `tools/msvc/optmode.py PROCESS.DLL src/process/map.cpp 0x100051f0 --modes base,s,t-off,i-off,y-off,a,w,g-off` wraps one function in `#pragma optimize`.

## What has been tried (all stay at 32 instructions)

* **Per-function optimisation modes** (`optmode.py`): base, `s` (size), `t` off, `i` off, `y` off, `a`, `w`, `g` off. None improve (size mode and `g` off are far worse). This function is speed-mode code; unlike SoundInit and DockWnd::DockWnd (see HANDOFF.md, "SoundInit and DockWnd::DockWnd MATCHED") it is not a size-mode function.
* **Arm order / condition:** `if (g_scale != 2) {scale-1 arm} else {scale-2 arm}`; `if (g_scale == 1) ...`.
* **Bit-test spelling:** `(*s++ != 0) ? 0x80 : 0` in one or both arms.
* **`*d = 0` placement:** before the `if` (arms start with `*d |=`), only in the then arm, only in the else arm, and the first bit written as plain `*d = *s++ ? 0x80 : 0`.
* **Declarations:** 8 arrangements of `bits`, `d`, `s`, `x`, `y` (order, split declaration + assignment, `s` before `d`, `y` before `x`).
* **Padding** (`pad_sh_k` externs before the function) 0-7.
* `typeshuffle.py` cannot parse this function (it reports 0 variants), so type/order shuffles were done by hand only.
* Earlier sessions (HANDOFF.md): scale-1-first branches, `*d = 0` hoisted before the `if`, explicit trailing `s++` per branch, `s[0]` form: all 32.

Specs and logs of this session: `build/scratch/s32/bc_spec.py`, `bc2_spec.py`, `bc3_spec.py` (and result dirs `bc`, `bc2`, `bc3`).

## Ideas not tried yet

1. Make the temporaries explicit: e.g. a `BYTE bit;` local per statement group, `BYTE a, b;` declared in the arms, or `register` temps, so the allocator sees 2 fixed temps in the scale-1 arm (the then-arm allocates fine, so compare what differs: the then-arm has a separate `s++;` statement between bits, the else-arm does not).
2. Give the else arm the same statement shape as the then arm, e.g. `*d |= *s ? 0x80 : 0; s++;` (separate `s++;` after each bit, with the pointer increments hoisted the same way as in the then-arm).
3. Express the 8 bits as a small inline helper / macro so each bit is its own expression statement, or a `for (bit = 0x80; bit; bit >>= 1)` loop that the compiler unrolls (it will not unroll; but the original may have been written as 8 explicit statements for scale 1 and a different form for scale 2).
4. Sweep padding much wider (0-63) and combine with idea 1-3 (position-dependent register roles: `docs/DECOMP.md`).
5. Compare with how the original handles the same `*d |= *s++ ? mask : 0` pattern in other functions that already match (search `src/process` and `src/lemmings` for the pattern).
6. Check whether the original's row/column loops are `for (y...)` / `for (x...)` at all: the original loop counters are `edi` (columns = (w+7)>>3, decremented) and `[esp+0x10]` (rows, decremented); ours match that, so the loop structure is probably right.

## Constraints and rules

* Never commit, publish or copy out the compiler (`build/vc20`, `vc21`, `vc22`) or the game files; the game code is the user's.
* Check `ps aux | grep -E "shapes.py|build.py|padplan"` before starting searches; do not run Wine gameplay tests while compile searches run.
* After an exact match: whole-file try.sh (re-pad with padplan if a neighbour loses its match) -> `build.py --funcs PROCESS` and diff against `build/scratch/s32/si/process_fo.log` (only BuildCollision may change) -> `make` -> update HANDOFF.md and `python3 build/scratch/progress_table.py RENDER=... PROCESS=... LEMMINGS=...`.
* New reusable scripts go in `tools/`, not scratch.
* Keep the MinGW build working: anything MSVC-specific goes under `#ifdef _MSC_VER`.

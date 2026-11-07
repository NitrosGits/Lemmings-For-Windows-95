#!/bin/zsh
# One scripted run of the game under Wine, with the harness PROCESS.DLL proxy:
#   MODS="VSMEM RENDER" tools/wine/run.sh NAME UPDATES 'harness commands;...' [FUN]
# MODS lists the rebuilt modules (from build/out) to use in place of the
# originals. Plays level 1 from the main menu, or Fun level FUN through Choose
# Level, runs the commands, waits for UPDATES lemming updates, and keeps the
# lemming log, command log and final screenshot in build/runs/NAME.*
# HD=1 (the default) plays in high resolution (Resolution 2) in a 1278-wide
# window; HD=0 in standard resolution, 638 wide. Each mode has its own
# baselines (HD: hd-base-l1, hd-base-dig; standard: base-l1, dig-base), and
# its own click positions (digger run, HD: 'waitupd 200;click 332 395;
# waitupd 228;click 596 138'; standard: 'waitupd 200;click 332 235;
# waitupd 228;click 288 70').
# Runs are muted by default (Wine audio driver off); SOUND=1 plays sound.
# SILENT=1 also minimizes the window, so you can keep working; the logs still
# MATCH the baselines.
ROOT=${0:A:h:h:h}
G=$ROOT/build/run/game
export WINEPREFIX=$ROOT/build/wineprefix WINEDEBUG=-all
wineserver -k 2>/dev/null
if [[ ${HD:-1} == 1 ]]; then res=2 width=1278 dx=320; else res=1 width=638 dx=0; fi
for v in "Resolution $res" "Width $width"; do
  wine reg add 'HKLM\Software\Visual Sciences\Windows Lemmings\Users\Tester' /v ${v% *} /t REG_DWORD /d ${v#* } /f /reg:32 >/dev/null 2>&1
done
if [[ ${SOUND:-0} == 1 ]]; then
  wine reg delete 'HKCU\Software\Wine\Drivers' /v Audio /f >/dev/null 2>&1
else
  wine reg add 'HKCU\Software\Wine\Drivers' /v Audio /t REG_SZ /d '' /f >/dev/null 2>&1
fi
if [[ ${SILENT:-0} == 1 ]]; then launch="wine start /min LEMMINGS.EXE"; else launch="wine LEMMINGS.EXE"; fi
wineserver -w
mkdir -p $G $ROOT/build/runs
rsync -a --delete --exclude MANUALS --exclude PROCESS.DLL $ROOT/orig/WINLEMM/ $G/
cp $ROOT/orig/WINLEMM/PROCESS.DLL $G/PROCESS0.DLL
for m in ${=MODS}; do
  if [[ $m == PROCESS ]]; then cp $ROOT/build/out/PROCESS.DLL $G/PROCESS0.DLL
  elif [[ $m == LEMMINGS ]]; then cp $ROOT/build/out/LEMMINGS.EXE $G/
  else cp $ROOT/build/out/$m.DLL $G/; fi
done
cp $ROOT/build/harness/PROCESS.DLL $G/PROCESS.DLL
cd $G
rm -f CMD.TXT CMDLOG.TXT LEMLOG.TXT *.BMP(N)
if [ -n "$4" ]; then
  start="click $((380 + dx)) 232;wait 2500;dbl 60 70;wait 1000;dbl 60 86;wait 1000;click 120 $((86 + 16 * $4));wait 800;click 465 34"
else
  start="click $((243 + dx)) 232"
fi
{ echo "wait 8000"; echo "${start//;/\n}"; echo "${3//;/\n}"; echo "waitupd $2"; echo "shot end"; } > CMD.TXT
${=launch} > $ROOT/build/runs/$1.wine.log 2>&1 &
t=0
until grep -q "shot end" CMDLOG.TXT 2>/dev/null || [ $t -gt ${TIMEOUT:-300} ]; do sleep 1; t=$((t+1)); done
wineserver -k 2>/dev/null
cp LEMLOG.TXT $ROOT/build/runs/$1.txt 2>/dev/null
cp CMDLOG.TXT $ROOT/build/runs/$1.cmd 2>/dev/null
[ -f end.BMP ] && cp end.BMP $ROOT/build/runs/$1.bmp
echo "run $1 (MODS=$MODS): $(grep -c '' $ROOT/build/runs/$1.txt 2>/dev/null) log lines, ${t}s"

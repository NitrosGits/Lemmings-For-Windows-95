#!/bin/zsh
# One scripted run of the 64-bit Windows build under Wine, the counterpart of
# tools/wine/run.sh (the 32-bit game) and tools/port/run.sh (macOS):
#   tools/port/winrun.sh NAME UPDATES 'harness commands;...' [BASELINE]
# Plays level 1 from the title screen with the Wine runs' player "Tester"
# (Resolution 2, a 1278-wide window: HD=1, the default; HD=0 standard), runs
# the commands, waits for UPDATES lemming updates, and keeps the lemming log,
# command log and final screenshot in build/runs-win64/NAME.*. With a
# BASELINE (e.g. build/runs/hd-base-l1.txt) the log is compared with it by
# tools/cmplog.py. Muted unless SOUND=1. Needs the build
# (build/win64/Lemmings64.exe, or EXE=) and the original data (ORIG, default
# orig/WINLEMM, else the main checkout's). It has its own Wine prefix
# (build/wineprefix64), so the 32-bit runs' is left alone. LEVEL= starts
# another level (default 1); PLAYERDB= uses another player database; START=
# replaces the click that starts the level (START= alone stays on the title
# screen).
ROOT=${0:A:h:h:h}
MAIN=${ROOT:h:h:h}                      # the main checkout, when this is a worktree
EXE=${EXE:-$ROOT/build/win64/Lemmings64.exe}
ORIG=${ORIG:-$ROOT/orig/WINLEMM}
[[ -d $ORIG ]] || ORIG=$MAIN/orig/WINLEMM
ORIG=${ORIG:A}
OUT=$ROOT/build/runs-win64
R=$OUT/$1.run
export WINEPREFIX=${WINEPREFIX:-$ROOT/build/wineprefix64} WINEDEBUG=${WINEDEBUG:--all}
win() { print -r -- "Z:${1//\//\\}" }   # a Unix path as Wine sees it
reg() { print -r -- "${$(win $1)//\\/\\\\}" }   # ... inside a .reg file's string

wineserver -k 2>/dev/null
if [[ ${HD:-1} == 1 ]]; then res=2 width=1278 dx=320; else res=1 width=638 dx=0; fi
mkdir -p $OUT
rm -rf "${R:?}"
mkdir -p $R/user/WinLemm
# the 32-bit Wine prefix's player database, if there is one (the same progress)
DB="$MAIN/build/wineprefix/drive_c/Program Files (x86)/WinLemm/usr00001.db"
[[ -f $DB ]] || DB="$ROOT/build/wineprefix/drive_c/Program Files (x86)/WinLemm/usr00001.db"
[[ -n $PLAYERDB ]] && DB=$PLAYERDB
[[ -f $DB ]] && cp "$DB" $R/user/WinLemm/usr00001.db
# the game's settings: the user's own registry key (port/windows/win64.c)
cat > $R/tester.reg <<EOF
REGEDIT4

[-HKEY_CURRENT_USER\\Software\\Visual Sciences]

[HKEY_CURRENT_USER\\Software\\Visual Sciences\\Windows Lemmings]
"Params"="ACAMAAABAEBO"
"Registered"="Tester"

[HKEY_CURRENT_USER\\Software\\Visual Sciences\\Windows Lemmings\\Users]
"LastUser"="Tester"

[HKEY_CURRENT_USER\\Software\\Visual Sciences\\Windows Lemmings\\Users\\Tester]
"DatabaseName"="$(reg $R/user/WinLemm/usr00001.db)"
"Height"=dword:000001d7
"IconDocked"=dword:0000000${ICONDOCKED:-1}
"IconX"=dword:00000000
"IconY"=dword:00000000
"Level"=dword:$(printf %08x ${LEVEL:-1})
"MapDocked"=dword:00000001
"MapX"=dword:00000000
"MapY"=dword:00000000
"Maximised"=dword:00000000
"Music"=dword:00000001
"PauseFocus"=dword:00000000
"Resolution"=dword:$(printf %08x $res)
"SoundFX"=dword:00000001
"VisualFX"=dword:00000001
"Width"=dword:$(printf %08x $width)
"Zoom"=dword:00000001

[HKEY_CURRENT_USER\\Software\\Wine\\Drivers]
$([[ ${SOUND:-0} == 1 ]] && print '"Audio"=-' || print '"Audio"=""')
EOF
wine regedit /S "$(win $R/tester.reg)" > $R/regedit.log 2>&1
wineserver -w
start=${START-"click $((243 + dx)) 232"}
{ echo "wait 8000"; echo "${start//;/\n}"; [[ -n $3 ]] && echo "${3//;/\n}"; echo "waitupd $2"; echo "shot end"; } > $R/CMD.TXT
LEMMINGS_HARNESS=1 LEMMINGS_HARNESS_DIR="$(win $R)" LEMMINGS_USERDIR="$(win $R/user)" \
  wine $EXE --data "$(win $ORIG)" > $R/wine.log 2>&1 &
pid=$!
t=0
until grep -q "shot end" $R/CMDLOG.TXT 2>/dev/null || [ $t -gt ${TIMEOUT:-300} ] || ! kill -0 $pid 2>/dev/null; do sleep 1; t=$((t+1)); done
if kill -0 $pid 2>/dev/null; then how="stopped by winrun.sh"; else wait $pid; how="exited with code $?"; fi
wineserver -k 2>/dev/null
cp $R/LEMLOG.TXT $OUT/$1.txt 2>/dev/null
cp $R/CMDLOG.TXT $OUT/$1.cmd 2>/dev/null
[ -f $R/end.BMP ] && cp $R/end.BMP $OUT/$1.bmp
echo "run $1: $(grep -c '' $OUT/$1.txt 2>/dev/null) log lines, ${t}s, game $how"
if [[ -n $4 ]]; then
  python3 $ROOT/tools/cmplog.py $4 $OUT/$1.txt | tail -5
fi

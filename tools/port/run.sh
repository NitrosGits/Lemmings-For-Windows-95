#!/bin/zsh
# One scripted run of the native build, the counterpart of tools/wine/run.sh:
#   tools/port/run.sh NAME UPDATES 'harness commands;...' [BASELINE]
# Plays level 1 from the title screen with the Wine runs' player "Tester"
# (Resolution 2, a 1278-wide window: HD=1, the default; HD=0 standard), runs
# the commands, waits for UPDATES lemming updates, and keeps the lemming log,
# command log and final screenshot in build/runs-native/NAME.*. With a
# BASELINE (e.g. build/runs/hd-base-l1.txt) the log is compared with it by
# tools/cmplog.py. Muted unless SOUND=1. Needs the native build
# (build/native/lemmings) and the original data (ORIG, default orig/WINLEMM,
# else the main checkout's). LEVEL= starts another level (default 1);
# PLAYERDB= uses another player database; ICONDOCKED=0 starts with the skill
# panel in the upper dock (above the level) instead of the lower one.
ROOT=${0:A:h:h:h}
EXE=${EXE:-$ROOT/build/native/lemmings}
ORIG=${ORIG:-$ROOT/orig/WINLEMM}
[[ -d $ORIG ]] || ORIG=${ROOT:h:h:h}/orig/WINLEMM
OUT=$ROOT/build/runs-native
R=$OUT/$1.run
# Wine's Width includes a 4-pixel frame each side; here the window is its client
if [[ ${HD:-1} == 1 ]]; then res=2 width=1270 dx=320; else res=1 width=630 dx=0; fi
mkdir -p $OUT
rm -rf "${R:?}"
mkdir -p "$R/prefs/drive_c/Program Files/WinLemm"
# the Wine prefix's player database, if there is one (the same progress)
DB="${ROOT:h:h:h}/build/wineprefix/drive_c/Program Files (x86)/WinLemm/usr00001.db"
[[ -f $DB ]] || DB="$ROOT/build/wineprefix/drive_c/Program Files (x86)/WinLemm/usr00001.db"
[[ -n $PLAYERDB ]] && DB=$PLAYERDB     # another player database (e.g. more levels done)
[[ -f $DB ]] && cp "$DB" "$R/prefs/drive_c/Program Files/WinLemm/usr00001.db"
cat > $R/prefs/registry.txt <<EOF
[HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion]
"ProgramFilesDir"="C:\\\\Program Files"

[HKEY_LOCAL_MACHINE\\Software\\Visual Sciences\\Windows Lemmings]
"Params"="ACAMAAABAEBO"
"Registered"="Tester"

[HKEY_LOCAL_MACHINE\\Software\\Visual Sciences\\Windows Lemmings\\Users]
"LastUser"="Tester"

[HKEY_LOCAL_MACHINE\\Software\\Visual Sciences\\Windows Lemmings\\Users\\Tester]
"DatabaseName"="C:\\\\Program Files\\\\WinLemm\\\\usr00001.db"
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
EOF
start=${START-"click $((243 + dx)) 232"}   # START= stays on the title screen
{ echo "wait 3000"; echo "${start//;/\n}"; [[ -n $3 ]] && echo "${3//;/\n}"; echo "waitupd $2"; echo "shot end"; } > $R/CMD.TXT
LEMMINGS_HARNESS=1 LEMMINGS_HARNESS_DIR=$R LEMMINGS_PREFS=$R/prefs LEMMINGS_SOUND=${SOUND:-0} LEMMINGS_MUSIC=${SOUND:-0} \
  $EXE --data $ORIG > $R/stderr.txt 2>&1 &
pid=$!
t=0
until grep -q "shot end" $R/CMDLOG.TXT 2>/dev/null || [ $t -gt ${TIMEOUT:-300} ] || ! kill -0 $pid 2>/dev/null; do sleep 1; t=$((t+1)); done
if kill -0 $pid 2>/dev/null; then kill $pid; wait $pid 2>/dev/null; how="stopped by run.sh"
else wait $pid; how="exited with code $?"; fi
cp $R/LEMLOG.TXT $OUT/$1.txt 2>/dev/null
cp $R/CMDLOG.TXT $OUT/$1.cmd 2>/dev/null
[ -f $R/end.BMP ] && cp $R/end.BMP $OUT/$1.bmp
echo "run $1: $(grep -c '' $OUT/$1.txt 2>/dev/null) log lines, ${t}s, game $how"
if [[ -n $4 ]]; then
  python3 $ROOT/tools/cmplog.py $4 $OUT/$1.txt | tail -5
fi

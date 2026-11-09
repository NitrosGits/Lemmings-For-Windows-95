#!/bin/zsh
# The native build's checks (PORTING.md, section 8), one line each:
#   tools/port/checks.sh [NAME...]        (default: all)
# EXE= picks the binary (default build/native/lemmings); BASE= the folder of
# the original game's reference logs (default the main checkout's build/runs).
# Gameplay runs compare their lemming logs with the original's; the menu runs
# (File > Exit and the application menu's Quit, through the system menu bar)
# check the exit code, the zoom run the zoomed frame's height.
ROOT=${0:A:h:h:h}
RUN=$ROOT/tools/port/run.sh
BASE=${BASE:-$ROOT/build/runs}
[[ -f $BASE/hd-base-l1.txt ]] || BASE=${ROOT:h:h:h}/build/runs
DIG='waitupd 200;click 332 395;waitupd 228;click 596 138'
NUKE='waitupd 100;click 552 395;waitupd 110;click 552 395;waitupd 120;dbl 552 395'
names=(l1 dig nuke sdig skey smenu squit zoomclk); (( $# )) && names=($@)

result() {   # the comparison's verdict, or the run's last line
  local out=$1
  print -r -- "$out" | grep -E '^(MATCH|[0-9]+\+ differences)' | head -1 | read -r v || v=$(print -r -- "$out" | tail -1)
  print -r -- "$v"
}

for n in $names; do
  case $n in
  l1)   r=$(TIMEOUT=90 $RUN l1 300 '' $BASE/hd-base-l1.txt) ;;
  dig)  r=$(TIMEOUT=120 $RUN dig 400 "$DIG" $BASE/hd-base-dig.txt) ;;
  nuke) r=$(TIMEOUT=60 $RUN nuke 264 "$NUKE" $BASE/s43-nuke-orig2.txt) ;;
  sdig) r=$(TIMEOUT=120 $RUN sdig 400 "${DIG//click/sclick}" $BASE/hd-base-dig.txt) ;;
  skey) r=$(TIMEOUT=120 $RUN skey 400 'waitupd 200;skey 8;waitupd 228;sclick 596 138' $BASE/hd-base-dig.txt) ;;
  smenu) r=$(START= TIMEOUT=40 $RUN smenu 0 'wait 500;smenu File/Exit;wait 3000' | tail -1) ;;
  squit) r=$(START= TIMEOUT=40 $RUN squit 0 'wait 500;smenu App/Quit Lemmings;wait 3000' | tail -1) ;;
  zoomclk)
    r=$(TIMEOUT=60 $RUN zoomclk 0 'waitupd 20;menu Set Options;wait 1500;sclick 251 104;wait 800;ctl OK;wait 3000;shot z1;menu Exit;wait 2000' | tail -1)
    r="$r, $(sips -g pixelHeight $ROOT/build/runs-native/zoomclk.run/z1.BMP 2>/dev/null | grep -o 'pixelHeight.*')"
    ;;
  *) r="unknown check" ;;
  esac
  print -r -- "$n: $(result "$r")"
done

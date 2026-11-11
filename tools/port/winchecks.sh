#!/bin/zsh
# The 64-bit Windows build's checks (PORTING.md, section 10), one line each:
#   tools/port/winchecks.sh [NAME...]        (default: all)
# Every run plays the game under Wine (tools/port/winrun.sh) and compares its
# lemming log with a log of the original 32-bit game, made by
# tools/wine/run.sh and tools/wine/suite.sh: BASE= is their folder (default
# build/runs, else the main checkout's). EXE= picks the program.
#   l1, dig, nuke, ff      level 1 untouched, a digger, the nuke, fast forward
#                          (high resolution, as the macOS port's checks)
#   bash block bomb build  one skill each on level 1, the release rate, and
#   sdig float mine rate   the digger again (standard resolution: the suite's
#                          own logs, tools/wine/suite.sh)
#   f2                     Fun 2, "Only floaters can survive this", chosen in the
#                          Choose Level dialog, four floaters given: against
#                          s45-f2-float.txt (the original: tools/wine/run.sh
#                          s45-f2-float 330 "<the same commands>" 2)
#   dialogs                the Options, Select Player, Choose Level and About
#                          dialogs opened from the title screen: their
#                          controls (class, id, text, state) as
#                          tools/port/expect/win-dialogs.txt has them
#   exit                   File > Exit from the title screen: exit code 0
ROOT=${0:A:h:h:h}
RUN=$ROOT/tools/port/winrun.sh
BASE=${BASE:-$ROOT/build/runs}
[[ -f $BASE/hd-base-l1.txt ]] || BASE=${ROOT:h:h:h}/build/runs
typeset -A S        # the suite's scenarios (standard resolution)
S[sdig]='waitupd 200;click 332 235;waitupd 228;click 288 70'
S[build]='waitupd 200;click 200 235;waitupd 228;click 288 70'
S[bash]='waitupd 200;click 244 235;waitupd 228;click 288 70'
S[block]='waitupd 200;click 156 235;waitupd 228;click 288 70'
S[bomb]='waitupd 200;click 112 235;waitupd 228;click 288 70'
S[mine]='waitupd 200;click 288 235;waitupd 228;click 288 70'
S[float]='waitupd 150;click 68 235;waitupd 160;click 288 70'
S[rate]='waitupd 20;click 376 235;click 376 235;click 376 235;waitupd 100;click 420 235'
F2='waitupd 30;click 68 395;waitupd 65;click 502 96;waitupd 93;click 502 96;waitupd 121;click 502 96;waitupd 149;click 502 96'
F2START='click 700 232;wait 2500;dbl 60 70;wait 1000;dbl 60 86;wait 1000;click 120 118;wait 800;click 465 34'
names=(l1 dig nuke ff bash block bomb build sdig float mine rate f2 dialogs exit); (( $# )) && names=($@)
fail=0

result() {   # the comparison's verdict, or the run's last line
  local v
  print -r -- "$1" | grep -E '^(MATCH|[0-9]+\+ differences)' | head -1 | read -r v || v=$(print -r -- "$1" | tail -1)
  print -r -- "$v"
}

for n in $names; do
  case $n in
  l1)   r=$(result "$(TIMEOUT=120 $RUN l1 300 '' $BASE/hd-base-l1.txt)") ;;
  dig)  r=$(result "$(TIMEOUT=120 $RUN dig 400 'waitupd 200;click 332 395;waitupd 228;click 596 138' $BASE/hd-base-dig.txt)") ;;
  nuke) r=$(result "$(TIMEOUT=120 $RUN nuke 262 'waitupd 100;click 552 395;waitupd 110;click 552 395;waitupd 120;dbl 552 395' $BASE/s43-nuke-orig2.txt)") ;;
  ff)   r=$(result "$(TIMEOUT=120 $RUN ff 300 'waitupd 20;click 510 395' $BASE/hd-base-ff.txt)") ;;
  bash|block|bomb|build|sdig|float|mine|rate)
    r=$(result "$(HD=0 TIMEOUT=120 $RUN $n 400 "${S[$n]}" $BASE/suite-${n#s}-base.txt)") ;;
  f2)   r=$(result "$(START=$F2START TIMEOUT=150 $RUN f2 330 "$F2" $BASE/s45-f2-float.txt)") ;;
  dialogs)
    r=$(START= TIMEOUT=90 $RUN dialogs 0 'wait 1500;menu Set Options;wait 1500;ctls;ctl Cancel;wait 800;menu Select Player;wait 1500;ctls;ctl Cancel;wait 800;menu Choose Level;wait 2500;ctls;ctl Cancel;wait 800;menu About Lemmings;wait 1500;ctls;ctl OK;wait 800;menu Exit;wait 2000' | tail -1)
    # (the About box's system information changes from machine to machine)
    grep '^  ctl class' $ROOT/build/runs-win64/dialogs.cmd | tr -d '\r' | grep -v "id 283 " > $ROOT/build/runs-win64/dialogs.ctls
    if cmp -s $ROOT/tools/port/expect/win-dialogs.txt $ROOT/build/runs-win64/dialogs.ctls; then r="MATCH ($r)"
    else r="DIFFER: diff tools/port/expect/win-dialogs.txt build/runs-win64/dialogs.ctls ($r)"; fi
    ;;
  exit)
    r=$(START= TIMEOUT=60 $RUN exit 0 'wait 1500;menu Exit;wait 3000' | tail -1)
    [[ $r == *"exited with code 0" ]] && r="MATCH ($r)"
    ;;
  *) r="unknown check" ;;
  esac
  print -r -- "$n: $r"
  [[ $r == MATCH* ]] || fail=1
done
exit $fail

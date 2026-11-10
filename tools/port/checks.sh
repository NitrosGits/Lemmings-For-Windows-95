#!/bin/zsh
# The native build's checks (PORTING.md, section 8), one line each:
#   tools/port/checks.sh [NAME...]        (default: all)
# EXE= picks the binary (default build/native/lemmings); BASE= the folder of
# the original game's reference logs (default the main checkout's build/runs).
# Gameplay runs compare their lemming logs with the original's; the menu runs
# (smenu: a File item and the application menu's Settings through the system
# menu bar, squit: its Quit) and `again` (three levels, two replays from the
# end screen) check the exit code, the zoom run the zoomed frame's height;
# dialog runs compare the controls' states (and for the key runs the setting
# saved) with tools/port/expect/NAME.txt. `bar` compares what the skill
# panel's and the status bar's system views show while the digger is chosen
# and used, and `look` switches the system's look to dark and back and checks
# that the colours the game draws in followed. `sound` (not in the default
# list: it plays out loud) runs the nuke with sound and music on and measures
# what reached the output (the harness's `levels`).
ROOT=${0:A:h:h:h}
RUN=$ROOT/tools/port/run.sh
BASE=${BASE:-$ROOT/build/runs}
[[ -f $BASE/hd-base-l1.txt ]] || BASE=${ROOT:h:h:h}/build/runs
DIG='waitupd 200;click 332 395;waitupd 228;click 596 138'
NUKE='waitupd 100;click 552 395;waitupd 110;click 552 395;waitupd 120;dbl 552 395'
names=(l1 dig nuke sdig skey smenu squit zoomclk opts user level ukey okey oret scroll bar look again); (( $# )) && names=($@)

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
  smenu)
    # Exit and Set Options are not in File on a Mac: Quit and Settings in the application menu
    r=$(START= TIMEOUT=60 $RUN smenu 0 'wait 500;smenu File/Choose Level;wait 2500;ctl Cancel;wait 800;smenu App/Settings…;wait 1500;ctl Cancel;wait 800;smenu File/Exit;menu Exit;wait 3000' | tail -1)
    c=$ROOT/build/runs-native/smenu.cmd
    if [[ $(grep -c 'smenu .*: chosen' $c) != 2 || $(grep -c "ctl 'Cancel'" $c) != 2 ]] || ! grep -q 'smenu File/Exit: not found' $c
    then r="$r, MENUS DIFFER: see $c"; fi
    ;;
  squit) r=$(START= TIMEOUT=40 $RUN squit 0 'wait 500;smenu App/Quit Lemmings;wait 3000' | tail -1) ;;
  opts|user|level|ukey|okey|oret)
    # a dialog driven by real clicks and keys; its controls' states (ctls) as
    # tools/port/expect/NAME.txt has them. ukey: Return in the name field
    # (the new player is saved); okey: Escape cancels; oret: Return is OK
    case $n in
    opts) c='menu Set Options;wait 1500;ctls;sclick 36 56;wait 500;sclick 200 40;wait 500;ctls;ctl Cancel;wait 1000' ;;
    user) c='menu Select Player;wait 1500;ctls;sclick 60 60;wait 300;skey x;skey y;wait 500;ctls;sclick 60 100;wait 500;ctls;ctl Cancel;wait 1000' ;;
    level) c='menu Choose Level;wait 2500;ctls;sdbl 60 70;wait 1000;sdbl 60 86;wait 1000;sclick 120 102;wait 1500;ctls;ctl Cancel;wait 1000' ;;
    ukey) c='menu Select Player;wait 1500;sclick 60 60;wait 300;skey x;skey Return;wait 1500;ctls' ;;
    okey|oret) c="menu Set Options;wait 1500;sclick 36 56;wait 300;skey ${${n/okey/Escape}/oret/Return};wait 1500;ctls" ;;
    esac
    r=$(START= TIMEOUT=60 $RUN $n 0 "$c;menu Exit;wait 1500" | tail -1)
    grep '^  ctl [A-Za-z]' $ROOT/build/runs-native/$n.cmd > $ROOT/build/runs-native/$n.ctls
    case $n in
    ukey) grep '"LastUser"' $ROOT/build/runs-native/$n.run/prefs/registry.txt >> $ROOT/build/runs-native/$n.ctls ;;
    okey|oret) grep '"Music"' $ROOT/build/runs-native/$n.run/prefs/registry.txt >> $ROOT/build/runs-native/$n.ctls ;;
    esac
    if [[ ! -f $ROOT/tools/port/expect/$n.txt ]]; then r="$r, no expected states (tools/port/expect/$n.txt)"
    elif cmp -s $ROOT/tools/port/expect/$n.txt $ROOT/build/runs-native/$n.ctls; then r="MATCH ($r)"
    else r="DIFFER: diff tools/port/expect/$n.txt build/runs-native/$n.ctls ($r)"; fi
    ;;
  zoomclk)
    r=$(TIMEOUT=60 $RUN zoomclk 0 'waitupd 20;menu Set Options;wait 1500;sclick 251 104;wait 800;ctl OK;wait 3000;shot z1;menu Exit;wait 2000' | tail -1)
    r="$r, $(sips -g pixelHeight $ROOT/build/runs-native/zoomclk.run/z1.BMP 2>/dev/null | grep -o 'pixelHeight.*')"
    ;;
  scroll)
    # the level view's scroll bar (the system's scroller) on level 20, whose
    # scenery spans the map: a click in the track (one page), the knob dragged
    # to the right end, the left end and the middle; the positions (sbs) as
    # tools/port/expect/scroll.txt has them
    r=$(LEVEL=20 TIMEOUT=60 $RUN scroll 0 'waitupd 15;sbs;sclick 1100 331;wait 600;sbs;sdragsb H 1269;wait 1200;sbs;sdragsb H 0;wait 1200;sbs;sdragsb H 635;wait 1200;sbs;menu Exit;wait 1500' | tail -1)
    grep '^  sb ' $ROOT/build/runs-native/scroll.cmd > $ROOT/build/runs-native/scroll.ctls
    if cmp -s $ROOT/tools/port/expect/scroll.txt $ROOT/build/runs-native/scroll.ctls; then r="MATCH ($r)"
    else r="DIFFER: diff tools/port/expect/scroll.txt build/runs-native/scroll.ctls ($r)"; fi
    ;;
  bar)
    # the skill panel and the status bar as system views: what they show at
    # the start, with the digger chosen, and after one was used (the clock
    # left out: the views are read between two updates)
    r=$(TIMEOUT=120 $RUN bar 400 "waitupd 190;views;${DIG/waitupd 228/waitupd 210;views;waitupd 228};waitupd 240;views" $BASE/hd-base-dig.txt)
    # (a button that is on is animated: its picture changes from frame to frame)
    grep '^  view ' $ROOT/build/runs-native/bar.cmd | sed -E "s/ 'Time: [0-9:]*'//; / on,/s/ with [0-9]* pixels//" > $ROOT/build/runs-native/bar.ctls
    if cmp -s $ROOT/tools/port/expect/bar.txt $ROOT/build/runs-native/bar.ctls; then r="$(result "$r"), views MATCH"
    else r="$(result "$r"), views DIFFER: diff tools/port/expect/bar.txt build/runs-native/bar.ctls"; fi
    ;;
  look)
    # the system's look changed while playing: the button face colour (15)
    # and the window text colour (8) the game draws in are dark's, then light's
    r=$(LEMMINGS_APPEARANCE=light TIMEOUT=60 $RUN look 60 'waitupd 20;syscolor 15;syscolor 8;look dark;wait 800;syscolor 15;syscolor 8;views;look light;wait 800;syscolor 15;syscolor 8' | tail -1)
    v=($(grep '^  syscolor' $ROOT/build/runs-native/look.cmd | awk '{print $3}'))
    if [[ $#v == 6 ]] && (( 0x${v[1]} > 0x${v[3]} && 0x${v[2]} < 0x${v[4]} )) && [[ $v[1] == $v[5] && $v[2] == $v[6] ]] &&
       (( $(grep -c '^  view icon' $ROOT/build/runs-native/look.cmd) == 13 ))
    then r="MATCH ($r; face $v[1] dark $v[3], text $v[2] dark $v[4])"; else r="LOOK DIFFERS: $v ($r)"; fi
    ;;
  sound)
    r=$(SOUND=1 TIMEOUT=90 $RUN sound 264 "waitupd 50;levels;${NUKE};waitupd 200;levels" $BASE/s43-nuke-orig2.txt)
    lv=$(grep 'levels:' $ROOT/build/runs-native/sound.cmd | tail -1)
    fx=$(print -r -- "$lv" | sed -E 's/.*effects [0-9.]+ s out, ([0-9.]+) s with sound.*/\1/')
    mo=$(print -r -- "$lv" | sed -E 's/.*music ([0-9.]+) s out.*/\1/')
    mh=$(print -r -- "$lv" | sed -E 's/.*music [0-9.]+ s out, ([0-9.]+) s with sound.*/\1/')
    # the four effects hold about 1.4 s of sound; the music plays without a gap
    if (( fx >= 1.2 && mh >= 10 && mh >= mo * 0.95 )); then v=heard; else v="NOT HEARD"; fi
    r="$(result "$r"), sound $v: effects ${fx} s, music ${mh} of ${mo} s"
    ;;
  again)
    crashes=$(ls ~/Library/Logs/DiagnosticReports 2>/dev/null | grep -ci lemmings)
    r=$(TIMEOUT=150 $RUN again 0 'waitupd 120;dbl 552 395;waitupd 264;wait 4000;shot es1;click 445 338;wait 3000;waitupd 330;shot l2;dbl 552 395;wait 22000;shot es2;click 445 338;wait 3000;waitupd 480;shot l3;menu Exit;wait 3000' | tail -1)
    (( $(ls ~/Library/Logs/DiagnosticReports 2>/dev/null | grep -ci lemmings) > crashes )) && r="$r, NEW CRASH REPORT"
    ;;
  *) r="unknown check" ;;
  esac
  print -r -- "$n: $(result "$r")"
done

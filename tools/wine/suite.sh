#!/bin/zsh
# Level-1 behaviour suite: each scenario runs once with the original binaries
# (baseline, kept in build/runs/suite-NAME-base.txt and reused) and once with
# the rebuilt modules in $MODS, and the lemming logs are compared.
#   MODS="VSMEM ... LEMMINGS" tools/wine/suite.sh [NAME...]
# Skill buttons sit at client x = 24 + 44 * i, y = 235 (0 climber, 1 floater,
# 2 bomber, 3 blocker, 4 builder, 5 basher, 6 miner, 7 digger, 8 plus,
# 9 minus, 10 pause, 11 fast forward, 12 nuke); (288, 70) is on a lemming at
# update 228. Never run this while compile searches load the machine: the
# runs are timing-sensitive.
ROOT=${0:A:h:h:h}
typeset -A S
S[dig]='waitupd 200;click 332 235;waitupd 228;click 288 70'
S[build]='waitupd 200;click 200 235;waitupd 228;click 288 70'
S[bash]='waitupd 200;click 244 235;waitupd 228;click 288 70'
S[block]='waitupd 200;click 156 235;waitupd 228;click 288 70'
S[bomb]='waitupd 200;click 112 235;waitupd 228;click 288 70'
S[mine]='waitupd 200;click 288 235;waitupd 228;click 288 70'
S[float]='waitupd 150;click 68 235;waitupd 160;click 288 70'
S[rate]='waitupd 20;click 376 235;click 376 235;click 376 235;waitupd 100;click 420 235'
names=(${@:-${(ok)S}})
fail=0
for n in $names; do
  if [[ ! -f $ROOT/build/runs/suite-$n-base.txt ]]; then
    MODS= $ROOT/tools/wine/run.sh suite-$n-base 400 "${S[$n]}" >/dev/null
  fi
  $ROOT/tools/wine/run.sh suite-$n 400 "${S[$n]}" >/dev/null
  r=$(python3 $ROOT/tools/cmplog.py $ROOT/build/runs/suite-$n-base.txt $ROOT/build/runs/suite-$n.txt 2>&1 | tail -1)
  echo "$n: $r"
  [[ $r == MATCH ]] || fail=1
done
exit $fail

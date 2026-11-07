#!/bin/zsh
# Claude Code hook (see .claude/settings.json): keeps HANDOFF.md current.
#   handoff_hook.sh post   after a tool call: remind when HANDOFF.md is stale
#   handoff_hook.sh stop   at the end of a turn: block until it is updated
# Stale = older than 30 minutes while files in src/, tools/ or docs/ changed
# since. Reminders after tool calls come at most every 10 minutes.
ROOT=${0:A:h:h}
H=$ROOT/HANDOFF.md
STAMP=$ROOT/build/.handoff_reminded
input=$(cat)
[[ -f $H ]] || exit 0
now=$(date +%s)
age=$(( now - $(stat -f %m $H) ))
(( age < ${HANDOFF_MAXAGE:-1800} )) && exit 0
changed=$(find $ROOT/src $ROOT/tools $ROOT/docs -type f -newer $H 2>/dev/null | head -1)
[[ -z $changed ]] && exit 0
mins=$(( age / 60 ))
msg="HANDOFF.md is $mins minutes old and project files have changed since (e.g. ${changed#$ROOT/}). Update HANDOFF.md now with the current state (module/function counts, what changed, what was tried, next steps) so another session can pick up from it, then continue."
case $1 in
  post)
    if [[ -f $STAMP ]] && (( now - $(stat -f %m $STAMP) < 600 )); then exit 0; fi
    mkdir -p ${STAMP:h} && touch $STAMP
    jq -n --arg m "$msg" '{hookSpecificOutput: {hookEventName: "PostToolUse", additionalContext: $m}}'
    ;;
  stop)
    [[ $(print -r -- "$input" | jq -r '.stop_hook_active // false') == true ]] && exit 0
    jq -n --arg m "$msg" '{decision: "block", reason: $m}'
    ;;
esac
exit 0

#!/bin/zsh
# tools/spawn_codex.sh
# Non-interactive Codex runner for Antigravity-orchestrated tasks.
# Usage:
#   tools/spawn_codex.sh "Task prompt for Codex" [OUTPUT_LOG]
#
# Runs codex non-interactively with workspace-write permissions,
# bypassing git checks since the repo does not use git, and saves
# the final message to an output file for Antigravity to inspect.

ROOT=${0:A:h:h}
PROMPT="$1"
LOGFILE="${2:-$ROOT/build/scratch/codex/last_run.log}"
MSGOUT="$ROOT/build/scratch/codex/last_message.txt"

if [[ -z "$PROMPT" ]]; then
    echo "Usage: tools/spawn_codex.sh \"Prompt for Codex\" [LOGFILE]"
    exit 1
fi

mkdir -p "${LOGFILE:h}"
mkdir -p "${MSGOUT:h}"

echo "=== Spawning Codex CLI at $(date) ===" | tee "$LOGFILE"
echo "Prompt: $PROMPT" | tee -a "$LOGFILE"

# Run codex exec non-interactively
codex exec \
    --skip-git-repo-check \
    -s workspace-write \
    -C "$ROOT" \
    -o "$MSGOUT" \
    "$PROMPT" 2>&1 | tee -a "$LOGFILE"

EXIT_CODE=$?
echo "=== Codex exited with code $EXIT_CODE at $(date) ===" | tee -a "$LOGFILE"
exit $EXIT_CODE

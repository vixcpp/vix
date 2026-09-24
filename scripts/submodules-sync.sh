#!/usr/bin/env bash
set -euo pipefail

MODE="${1:-remote}"
echo "[Vix] sync submodules in mode: $MODE"
git submodule sync --recursive

if [[ "$MODE" == "remote" ]]; then
  git submodule update --init --remote --recursive
else
  git submodule update --init --recursive
fi

echo "[Vix] ✅ Sync terminé."

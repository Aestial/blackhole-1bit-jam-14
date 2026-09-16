#!/usr/bin/env bash
# =============================================================================
# build.sh — Convenience wrapper for scripts/build_and_run.sh
# =============================================================================
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${SCRIPT_DIR}/scripts/build_and_run.sh" "$@"

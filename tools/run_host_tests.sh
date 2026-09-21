#!/usr/bin/env bash
# DEPRECATED shim — superseded by the ntest orchestrator. Kept so existing
# hooks/CI/muscle-memory keep working. Runs: ntest host
exec "$(dirname "$0")/ntest" host "$@"

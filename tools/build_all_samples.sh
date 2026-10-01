#!/usr/bin/env bash
# DEPRECATED shim — superseded by tools/samples.sh. Kept so existing
# hooks/CI/muscle-memory keep working. Runs: tools/samples.sh m4
exec "$(dirname "$0")/samples.sh" m4 "$@"

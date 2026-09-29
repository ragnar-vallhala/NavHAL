#!/usr/bin/env bash
# DEPRECATED shim — superseded by tools/samples.sh. Kept so existing
# hooks/CI/muscle-memory keep working. Runs: tools/samples.sh avr
exec "$(dirname "$0")/samples.sh" avr "$@"

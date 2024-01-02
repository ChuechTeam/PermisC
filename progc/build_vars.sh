#!/usr/bin/env bash

set -e
set -u

# This script is used by the makefile to check if the build variables have changed
# (such as CC, CFLAGS, OPTIMIZE, etc.) and trigger a recompilation if so.

if [ "$#" -lt 2 ]; then
  echo "Wrong usage: build_vars.sh check|update <vars_file>"
  exit 2
fi

VAR_NAMES=("CC" "CFLAGS" "OPTIMIZE" "EXPERIMENTAL_ALGO" "EXPERIMENTAL_ALGO_AVX" "ASM")
print_vars() {
  for var in "${VAR_NAMES[@]}"; do
    if [ -v "$var" ]; then
      echo "$var=${!var}"
    fi
  done
}

VARS_FILE="$2"

case "$1" in
  check)
    if [ ! -f "$VARS_FILE" ]; then
      exit 1
    fi
    STORED=$(cat "$VARS_FILE")
    MINE=$(print_vars)
    if [ "$STORED" != "$MINE" ]; then
      exit 1
    else
      exit 0
    fi
    ;;
  update)
    print_vars > "$VARS_FILE"
    ;;
  *)
    echo "Wrong usage: build_vars.sh check|update <vars_file>"
    exit 2
esac
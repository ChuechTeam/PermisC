#!/usr/bin/env bash

PROGC_DIR="$(dirname "$0")"

if ! make -C "$PROGC_DIR" --no-print-directory -j build; then
  exit 1
fi

if [ "${DEBUG}" = '1' ]; then
  gdb "$PROGC_DIR/build-make/PermisC" -tui -q -ex=r --args "$PROGC_DIR/build-make/PermisC" "$@"
else
  "$PROGC_DIR/build-make/PermisC" "$@"
fi
#!/usr/bin/env bash

if ! make -j build; then
  exit 1
fi

if [ "${DEBUG}" = '1' ]; then
  gdb ./build-make/PermisC -tui -q -ex=r --args ./build-make/PermisC "$@"
else
  ./build-make/PermisC "$@"
fi
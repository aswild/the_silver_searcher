#!/bin/bash

if [[ $1 == -f ]] || [[ $1 == --force ]]; then
    FORCE_AUTOGEN=y
    shift
fi

set -e
cd "$(dirname "$0")"

if [[ $FORCE_AUTOGEN == y ]] || [[ ! -f ./configure ]]; then
    ./autogen.sh
fi
./configure "$@"
make -j$(nproc)

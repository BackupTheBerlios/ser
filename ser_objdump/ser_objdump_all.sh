#!/bin/bash

usage() {
cat <<EOF
Usage: $0 <module_root> [ser_objdump params]
EOF
}

if [ $# -eq 0 ]; then
    usage;
    exit 1
fi

ROOT="$1"
shift

find $ROOT -name "*.so" -exec ./ser_objdump "$@" "{}" ";"


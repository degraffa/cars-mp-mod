#!/bin/sh
# usage: ./build.sh [-DTESTAI]    needs: pip install ziglang
cd "$(dirname "$0")/src" && exec python3 -m ziglang cc -target x86-windows-gnu -O2 -shared "$@" -o ../binkw32.dll mod.c binkw32.def

#!/bin/bash
set -e
make -s
./build/emu/sra32emu "$@"

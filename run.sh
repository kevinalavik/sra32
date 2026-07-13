#!/bin/bash
set -e

# builds the emulator, tests and firmware and boots the firmware (no kernel or bootable device yet)
make -j$(nproc) -s
./build/emu/sra32emu -Ee build/firmware/sra32ka-firm-rev1 -u stdio -S sra32emu.log $@
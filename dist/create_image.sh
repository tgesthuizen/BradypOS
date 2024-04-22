#!/bin/bash
set -e

bs=1024
fs_base=$(echo -n '0x'; $OBJDUMP -x "$1" | grep __romfs_start | cut -d ' ' -f 1)
flash_base=0x10000000
fs_offset=$(( (fs_base - flash_base) / bs ))

dd if="$2" of="$4"
dd if="$3" of="$4" bs="$bs" seek="$fs_offset"

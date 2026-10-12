#!/bin/sh
# Buildroot post-image script: name the ISO and write its checksum.
# $1 = BINARIES_DIR
set -eu
BINARIES_DIR="$1"
ISO_IN="$BINARIES_DIR/rootfs.iso9660"
ISO_OUT="$BINARIES_DIR/CustomOS-x86_64.iso"

if [ ! -s "$ISO_IN" ]; then
    echo "post-image: $ISO_IN was not produced" >&2
    exit 1
fi
cp -f "$ISO_IN" "$ISO_OUT"
( cd "$BINARIES_DIR" && sha256sum "$(basename "$ISO_OUT")" > "$(basename "$ISO_OUT").sha256" )
size=$(du -h "$ISO_OUT" | cut -f1)
echo ">>> CustomOS image: $ISO_OUT ($size)"

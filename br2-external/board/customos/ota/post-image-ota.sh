#!/bin/sh
# Buildroot post-image script of customos_x86_64_ota_defconfig; runs after
# board/customos/post-image.sh.
#  * puts the system image (rootfs.squashfs, the same file the update bundles
#    carry) on the ISO as /customos/rootfs.squashfs for the A/B installer;
#  * writes customos-release.env with the version for release/make-release.sh.
# $1 = BINARIES_DIR
set -eu
BINARIES_DIR="$1"
ISO="$BINARIES_DIR/CustomOS-x86_64.iso"
SQUASHFS="$BINARIES_DIR/rootfs.squashfs"
XORRISO="$HOST_DIR/bin/xorriso"

die() { echo "post-image-ota: $*" >&2; exit 1; }
[ -s "$SQUASHFS" ] || die "$SQUASHFS was not produced (enable BR2_TARGET_ROOTFS_SQUASHFS)"
[ -s "$ISO" ] || die "$ISO was not produced (board/customos/post-image.sh must run first)"
[ -x "$XORRISO" ] || XORRISO=xorriso

( cd "$BINARIES_DIR" && sha256sum rootfs.squashfs > rootfs.squashfs.sha256 )

# Add the image to the ISO, keeping its BIOS and UEFI boot records as they are.
rm -f "$ISO.new"
"$XORRISO" -indev "$ISO" -outdev "$ISO.new" -boot_image any replay \
    -map "$SQUASHFS" /customos/rootfs.squashfs \
    -map "$SQUASHFS.sha256" /customos/rootfs.squashfs.sha256 \
    -commit >/dev/null 2>"$BINARIES_DIR/xorriso-ota.log" \
    || die "xorriso failed, see $BINARIES_DIR/xorriso-ota.log"
mv -f "$ISO.new" "$ISO"
( cd "$BINARIES_DIR" && sha256sum "$(basename "$ISO")" > "$(basename "$ISO").sha256" )

# Version information for the release tooling.
# shellcheck disable=SC1091
. "$TARGET_DIR/usr/lib/os-release"
cat > "$BINARIES_DIR/customos-release.env" <<EOF
VERSION_ID=$VERSION_ID
BUILD_ID=${BUILD_ID:-}
COMPATIBLE=$(sed -n 's/^compatible=//p' "$TARGET_DIR/usr/lib/customos/rauc-system.conf")
EOF
echo ">>> CustomOS $VERSION_ID: $ISO ($(du -h "$ISO" | cut -f1)), system image $(du -h "$SQUASHFS" | cut -f1)"
echo ">>> Create the signed update bundle with: release/make-release.sh $BINARIES_DIR"

#!/bin/bash
# build-images.sh: build the system images and releases for the A/B end-to-end
# test (tests/e2e/run-e2e.py). Ubuntu 24.04 host, run as root.
#
#   tests/e2e/build-images.sh --base DIR --kernel DIR --updater-build DIR --keys DIR \
#                             --evil-keys DIR --out DIR
#
#   --base           root file system made by mmdebstrap (see tests/e2e/README.md)
#   --kernel         extracted linux-image + linux-modules packages (boot/, lib/modules/)
#   --updater-build  CMake build directory of updater/ configured with -DCMAKE_INSTALL_PREFIX=/usr
#   --keys           test signing keys (release/keys/create-ota-keys.sh --no-passphrase --no-install)
#   --evil-keys      a second, untrusted set of keys
#
# IMPORTANT: this is a TEST FIXTURE. The base system is assembled from Ubuntu
# packages only so that CustomOS's real update components (customos-updated,
# customos-update, the preinit, GRUB menu, health check, RAUC hooks and
# configuration, and the A/B installer) can be exercised on real partitions,
# with real GRUB and RAUC, in QEMU. It is not CustomOS and is never shipped.
set -euo pipefail

BASE=""; KDIR=""; UPD=""; KEYS=""; EVIL=""; OUT=""
while [ $# -gt 0 ]; do
    case "$1" in
        --base) BASE="$2"; shift 2 ;;
        --kernel) KDIR="$2"; shift 2 ;;
        --updater-build) UPD="$2"; shift 2 ;;
        --keys) KEYS="$2"; shift 2 ;;
        --evil-keys) EVIL="$2"; shift 2 ;;
        --out) OUT="$2"; shift 2 ;;
        *) echo "unknown option $1" >&2; exit 2 ;;
    esac
done
for v in BASE KDIR UPD KEYS EVIL OUT; do [ -n "${!v}" ] || { echo "missing --${v,,}" >&2; exit 2; }; done
REPO="$(cd "$(dirname "$0")/../.." && pwd)"
HERE="$REPO/tests/e2e"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"

KVER=$(ls "$KDIR/lib/modules" | head -n1)
COMMON="$OUT/root-common"
echo ">>> common root file system ($KVER)"
rm -rf "$COMMON"
cp -a "$BASE" "$COMMON"

# Kernel inside the slot, where the A/B GRUB menu loads it from.
cp "$KDIR/boot/vmlinuz-$KVER" "$COMMON/boot/bzImage"
mkdir -p "$COMMON/usr/lib/modules"
cp -a "$KDIR/lib/modules/$KVER" "$COMMON/usr/lib/modules/"
# Only what a virtual machine needs, to keep the images small.
find "$COMMON/usr/lib/modules/$KVER/kernel" -mindepth 1 -maxdepth 1 \
    ! -name fs ! -name drivers ! -name crypto ! -name lib ! -name net -exec rm -rf {} +
rm -rf "$COMMON/usr/lib/modules/$KVER/kernel/drivers/"{gpu,media,net/wireless,infiniband,staging,iio,scsi/mpt3sas,usb/gadget}
depmod -b "$COMMON" "$KVER"

# CustomOS update components, exactly as built from updater/.
DESTDIR="$COMMON" cmake --install "$UPD" >/dev/null
install -D -m 0644 "$KEYS/ca.cert.pem" "$COMMON/etc/rauc/keyring.pem"
mkdir -p "$COMMON/data" "$COMMON/efi" "$COMMON/boot/customos"
echo 'set slot_cmdline="console=ttyS0,115200 loglevel=4"' > "$COMMON/boot/customos/cmdline.cfg"

# Test configuration: the mock GitHub on the host (QEMU user networking).
mkdir -p "$COMMON/etc/customos/update.conf.d"
cat > "$COMMON/etc/customos/update.conf.d/50-e2e.conf" <<'EOF'
[Source]
Repository=test/customos
ApiUrl=http://10.0.2.2:8077
[Updates]
AutomaticCheck=false
AllowInsecureHttp=true
EOF
cat > "$COMMON/etc/customos/health.conf" <<'EOF'
REQUIRED_UNITS="dbus.service rauc.service customos-updated.service customos-demo.service"
TRIAL_TIMEOUT=90
SETTLE=10
EOF
# Stand-in for the desktop (meridian-login.service): broken in one test version.
cat > "$COMMON/usr/local/bin/customos-demo" <<'EOF'
#!/bin/sh
if [ -e /usr/lib/customos-demo-broken ]; then
    echo "customos-demo: this version is broken on purpose" >&2
    exit 1
fi
exec sleep infinity
EOF
chmod +x "$COMMON/usr/local/bin/customos-demo"
cat > "$COMMON/etc/systemd/system/customos-demo.service" <<'EOF'
[Unit]
Description=Stand-in for the Meridian desktop (end-to-end test)
[Service]
ExecStart=/usr/local/bin/customos-demo
Restart=on-failure
RestartSec=2
[Install]
WantedBy=multi-user.target
EOF

# Console: root signs in automatically on the serial port (test driver).
mkdir -p "$COMMON/etc/systemd/system/serial-getty@ttyS0.service.d"
cat > "$COMMON/etc/systemd/system/serial-getty@ttyS0.service.d/autologin.conf" <<'EOF'
[Service]
ExecStart=
ExecStart=-/sbin/agetty --autologin root --noclear --keep-baud 115200 %I linux
EOF
mkdir -p "$COMMON/etc/systemd/network"
printf '[Match]\nName=en* eth*\n[Network]\nDHCP=yes\n' > "$COMMON/etc/systemd/network/20-wired.network"
chroot "$COMMON" systemctl enable systemd-networkd.service customos-demo.service \
    customos-updated.service customos-health-check.service customos-ota-setup.service \
    rauc.service serial-getty@ttyS0.service >/dev/null 2>&1
echo customos > "$COMMON/etc/hostname"
: > "$COMMON/etc/machine-id"
echo "root:x" | chroot "$COMMON" chpasswd -e 2>/dev/null || true

# Like the CustomOS image: a live user with autologin, its sudo rule and the
# live marker. Installed systems must drop all of them.
chroot "$COMMON" groupadd -r -g 990 live
chroot "$COMMON" useradd -r -u 990 -g live -G sudo -m -d /home/live -s /bin/bash -c "Live User" live 2>/dev/null \
    || chroot "$COMMON" useradd -r -u 990 -g live -m -d /home/live -s /bin/bash -c "Live User" live
chroot "$COMMON" groupadd -f -r wheel
chroot "$COMMON" groupadd -f -r audio; chroot "$COMMON" groupadd -f -r video
chroot "$COMMON" groupadd -f -r input; chroot "$COMMON" groupadd -f -r render
mkdir -p "$COMMON/etc/meridian" "$COMMON/etc/sudoers.d"
printf '# Meridian sign-in configuration.\nautologin = live\n' > "$COMMON/etc/meridian/login.conf"
printf 'live ALL=(ALL:ALL) NOPASSWD: ALL\n' > "$COMMON/etc/sudoers.d/live"
echo "This file marks the CustomOS live system. The installer does not copy it." > "$COMMON/etc/customos-live"

# ---- versions -------------------------------------------------------------------------
make_version() {   # make_version VERSION [broken|panic] [KEYS]
    local v=$1 kind=${2:-ok} keys=${3:-$KEYS} root="$OUT/root-$1"
    echo ">>> CustomOS $v ($kind)"
    rm -rf "$root"
    cp -a "$COMMON" "$root"
    cat > "$root/usr/lib/os-release" <<EOF
NAME="CustomOS"
PRETTY_NAME="CustomOS $v (test fixture)"
ID=customos
VERSION="$v"
VERSION_ID=$v
BUILD_ID="e2e-$v"
EOF
    ln -sf ../usr/lib/os-release "$root/etc/os-release"
    case "$kind" in
        broken) touch "$root/usr/lib/customos-demo-broken" ;;
        panic) printf '#!/bin/sh\necho "customos-preinit: broken on purpose"\nexit 1\n' > "$root/usr/lib/customos/customos-preinit" ;;
    esac
    mksquashfs "$root" "$OUT/rootfs-$v.squashfs" -comp xz -noappend -quiet -no-progress
    local img="$OUT/images-$v"
    rm -rf "$img"; mkdir -p "$img"
    ln "$OUT/rootfs-$v.squashfs" "$img/rootfs.squashfs"
    head -c 4096 /dev/zero > "$img/CustomOS-x86_64.iso"
    printf 'VERSION_ID=%s\nBUILD_ID=e2e-%s\nCOMPATIBLE=customos-x86_64-ab\n' "$v" "$v" > "$img/customos-release.env"
    "$REPO/release/make-release.sh" "$img" --out "$OUT/releases/v$v" --cert "$keys/signing.cert.pem" \
        --key "$keys/signing.key.pem" --keyring "$keys/ca.cert.pem" --expect-version "$v" \
        --changelog "$HERE/e2e-changelog.md" --rauc "$(command -v rauc)" >/dev/null
    rm -rf "$root"
}
make_version 0.2.0
make_version 0.3.0
make_version 0.4.0 broken
make_version 0.5.0 panic
make_version 0.6.0 ok "$EVIL"    # signed with an untrusted key

# ---- live installer environment ----------------------------------------------------------
echo ">>> live installer disk"
LIVE="$OUT/root-live"
rm -rf "$LIVE"
cp -a "$COMMON" "$LIVE"
printf 'NAME="CustomOS"\nPRETTY_NAME="CustomOS 0.2.0 (test fixture, live)"\nID=customos\nVERSION_ID=0.2.0\n' > "$LIVE/usr/lib/os-release"
ln -sf ../usr/lib/os-release "$LIVE/etc/os-release"
mkdir -p "$LIVE/opt/customos"
cp "$OUT/rootfs-0.2.0.squashfs" "$LIVE/opt/customos/rootfs.squashfs"
( cd "$LIVE/opt/customos" && sha256sum rootfs.squashfs > rootfs.squashfs.sha256 )
rm -f "$OUT/live.img"
mkfs.ext4 -q -F -L customos-live -d "$LIVE" "$OUT/live.img" 3G
rm -rf "$LIVE"
cp "$COMMON/boot/bzImage" "$OUT/bzImage"
rm -rf "$COMMON"
echo ">>> done: $OUT"
ls -la "$OUT" "$OUT/releases"

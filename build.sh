#!/usr/bin/env bash
# One-command build of CustomOS-x86_64.iso.
#
#   ./build.sh                 build on this machine (Debian/Ubuntu/Fedora host)
#   ./build.sh --docker        build inside a container (only Docker/Podman needed)
#   ./build.sh --menuconfig    adjust the Buildroot configuration, then build
#   ./build.sh --clean         remove the build output and start over
#   ./build.sh --ota [...]     build the A/B over-the-air update edition
#                              (customos_x86_64_ota_defconfig) in output-ota/;
#                              combine with the options above, e.g. --ota --docker
#
# Result: ./CustomOS-x86_64.iso  (plus .sha256)
#         --ota: ./CustomOS-v<VERSION>-x86_64.iso, and the system image for
#         release/make-release.sh in output-ota/images/
# A first build downloads ~3 GB of sources and takes 1-3 hours on 8 cores.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILDROOT_VERSION="2026.02.3"
BUILDROOT_DIR="${BUILDROOT_DIR:-$ROOT/buildroot}"
EXTERNAL="$ROOT/br2-external"
JOBS="${JOBS:-$(nproc)}"

# The OTA edition builds in its own output directory, so an existing build in
# output/ is never reconfigured or overwritten by it.
OTA=0
if [ "${1:-}" = "--ota" ]; then
    OTA=1
    shift
fi
if [ "$OTA" -eq 1 ]; then
    OUT="${OUT:-$ROOT/output-ota}"
    DEFCONFIG="customos_x86_64_ota_defconfig"
else
    OUT="${OUT:-$ROOT/output}"
    DEFCONFIG="customos_x86_64_defconfig"
fi

die() { echo "build.sh: $*" >&2; exit 1; }

docker_build() {
    local engine
    engine="$(command -v docker || command -v podman || true)"
    [ -n "$engine" ] || die "--docker needs docker or podman"
    "$engine" build -t customos-builder "$ROOT/docker"
    local ota_arg=()
    [ "$OTA" -eq 1 ] && ota_arg=(--ota)
    exec "$engine" run --rm -it \
        -v "$ROOT:/work" -w /work \
        -e JOBS="$JOBS" -e CUSTOMOS_BUILD_ID="${CUSTOMOS_BUILD_ID:-}" \
        --user "$(id -u):$(id -g)" \
        customos-builder ./build.sh "${ota_arg[@]}" "$@"
}

menuconfig=0
case "${1:-}" in
    --docker) shift; docker_build "$@" ;;
    --menuconfig) menuconfig=1 ;;
    --clean)
        echo "Removing $OUT"
        rm -rf -- "$OUT"
        exit 0 ;;
    -h|--help) sed -n '2,15p' "$0"; exit 0 ;;
    "") ;;
    *) die "unknown option $1 (see --help)" ;;
esac

[ "$(id -u)" -ne 0 ] || die "do not build as root (Buildroot refuses it); run as a normal user"

# ---- host requirements --------------------------------------------------------
missing=()
for t in make gcc g++ patch perl python3 rsync cpio unzip bc file wget git find sed awk bzip2 xz tar; do
    command -v "$t" >/dev/null 2>&1 || missing+=("$t")
done
if [ ${#missing[@]} -gt 0 ]; then
    echo "Missing host tools: ${missing[*]}"
    echo "Debian/Ubuntu: sudo apt install build-essential bc cpio file git libncurses-dev perl python3 rsync unzip wget xz-utils bzip2"
    echo "Or run: ./build.sh --docker"
    exit 1
fi

# ---- OTA prerequisites ----------------------------------------------------------
if [ "$OTA" -eq 1 ] && [ ! -s "$ROOT/keys/customos-ota-ca.cert.pem" ]; then
    echo "The OTA edition needs the public update certificate keys/customos-ota-ca.cert.pem."
    echo "Create the signing keys once with: release/keys/create-ota-keys.sh"
    echo "(see docs/SIGNING.md; the private keys must stay outside the repository)."
    exit 1
fi

# ---- Buildroot ------------------------------------------------------------------
if [ ! -d "$BUILDROOT_DIR/.git" ] && [ ! -f "$BUILDROOT_DIR/Makefile" ]; then
    echo ">>> Fetching Buildroot $BUILDROOT_VERSION"
    git clone --depth 1 --branch "$BUILDROOT_VERSION" \
        https://gitlab.com/buildroot.org/buildroot.git "$BUILDROOT_DIR" \
    || git clone --depth 1 --branch "$BUILDROOT_VERSION" \
        https://github.com/buildroot/buildroot.git "$BUILDROOT_DIR"
fi

mkdir -p "$OUT"
if [ ! -f "$OUT/.config" ]; then
    echo ">>> Configuring ($DEFCONFIG)"
    make -C "$BUILDROOT_DIR" O="$OUT" BR2_EXTERNAL="$EXTERNAL" "$DEFCONFIG"
fi
if [ "$menuconfig" -eq 1 ]; then
    make -C "$BUILDROOT_DIR" O="$OUT" menuconfig
fi

echo ">>> Building (jobs: $JOBS) - output in $OUT"
# Buildroot builds packages one at a time but each package uses all cores.
make -C "$BUILDROOT_DIR" O="$OUT" BR2_JLEVEL="$JOBS"

ISO="$OUT/images/CustomOS-x86_64.iso"
[ -s "$ISO" ] || die "the build finished but $ISO is missing; see the log above"
if [ "$OTA" -eq 1 ]; then
    NAME="CustomOS-v$(tr -d ' \n' < "$ROOT/VERSION")-x86_64.iso"
    cp -f "$ISO" "$ROOT/$NAME"
    ( cd "$ROOT" && sha256sum "$NAME" > "$NAME.sha256" )
    echo
    echo "Done: $ROOT/$NAME"
    cat "$ROOT/$NAME.sha256"
    echo "Signed update bundle and release files: release/make-release.sh $OUT/images (docs/RELEASING.md)"
    exit 0
fi
cp -f "$ISO" "$ISO.sha256" "$ROOT/"
echo
echo "Done: $ROOT/CustomOS-x86_64.iso"
cat "$ROOT/CustomOS-x86_64.iso.sha256"
echo "Boot it in a VM: see docs/VM-SETUP.md"

#!/bin/bash
# make-release.sh: turn a finished OTA build into the files of a GitHub release.
#
#   release/make-release.sh IMAGES_DIR [--out DIR] [--cert FILE --key FILE]
#                           [--keyring FILE] [--rauc RAUC] [--min-version X.Y.Z]
#                           [--channel stable] [--changelog FILE] [--expect-version X.Y.Z]
#
# IMAGES_DIR is output-ota/images (build.sh --ota). The signing certificate and
# key come from --cert/--key, or from the environment:
#   CUSTOMOS_SIGNING_CERT_FILE / CUSTOMOS_SIGNING_KEY_FILE   paths, or
#   CUSTOMOS_SIGNING_CERT / CUSTOMOS_SIGNING_KEY             PEM text (CI secrets)
#
# Produces in DIR (default release/out/vX.Y.Z):
#   CustomOS-vX.Y.Z.raucb            signed RAUC bundle (dm-verity) with the system image
#   CustomOS-vX.Y.Z-manifest.json    version, compatible, size, SHA-256, changelog
#   CustomOS-vX.Y.Z-x86_64.iso       live/installer ISO
#   SHA256SUMS                       checksums of the three files above
#   RELEASE_NOTES.md                 the CHANGELOG.md section, for the release page
# and checks the bundle against the trusted CA before finishing.
set -euo pipefail

die() { echo "make-release: $*" >&2; exit 1; }
REPO="$(cd "$(dirname "$0")/.." && pwd)"

IMAGES=""; OUT=""; CERT="${CUSTOMOS_SIGNING_CERT_FILE:-}"; KEY="${CUSTOMOS_SIGNING_KEY_FILE:-}"
KEYRING="$REPO/keys/customos-ota-ca.cert.pem"; RAUC=""; MIN_VERSION=""; CHANNEL="stable"
CHANGELOG="$REPO/CHANGELOG.md"; EXPECT_VERSION="$(tr -d ' \n' < "$REPO/VERSION")"
while [ $# -gt 0 ]; do
    case "$1" in
        --out) OUT="$2"; shift 2 ;;
        --cert) CERT="$2"; shift 2 ;;
        --key) KEY="$2"; shift 2 ;;
        --keyring) KEYRING="$2"; shift 2 ;;
        --rauc) RAUC="$2"; shift 2 ;;
        --min-version) MIN_VERSION="$2"; shift 2 ;;
        --channel) CHANNEL="$2"; shift 2 ;;
        --changelog) CHANGELOG="$2"; shift 2 ;;
        --expect-version) EXPECT_VERSION="$2"; shift 2 ;;
        -h|--help) sed -n '2,25p' "$0"; exit 0 ;;
        -*) die "unknown option $1" ;;
        *) [ -z "$IMAGES" ] || die "only one IMAGES_DIR"; IMAGES="$1"; shift ;;
    esac
done
[ -n "$IMAGES" ] && [ -d "$IMAGES" ] || die "usage: make-release.sh IMAGES_DIR (see --help)"
IMAGES="$(cd "$IMAGES" && pwd)"

# ---- inputs ---------------------------------------------------------------------------
[ -s "$IMAGES/customos-release.env" ] || die "$IMAGES/customos-release.env missing: build with build.sh --ota"
[ -s "$IMAGES/rootfs.squashfs" ] || die "$IMAGES/rootfs.squashfs missing"
[ -s "$IMAGES/CustomOS-x86_64.iso" ] || die "$IMAGES/CustomOS-x86_64.iso missing"
# shellcheck disable=SC1091
. "$IMAGES/customos-release.env"
VERSION="$VERSION_ID"
[ "$VERSION" = "$EXPECT_VERSION" ] || die "the image is $VERSION but VERSION says $EXPECT_VERSION; rebuild"
if [ -n "${GITHUB_REF_NAME:-}" ] && [ "${GITHUB_REF_TYPE:-}" = tag ] && [ "$GITHUB_REF_NAME" != "v$VERSION" ]; then
    die "tag $GITHUB_REF_NAME does not match VERSION $VERSION"
fi
[ -n "${COMPATIBLE:-}" ] || die "no compatible string in customos-release.env"
case "$CHANNEL" in stable|beta) ;; *) die "--channel must be stable or beta" ;; esac
if [ "$CHANNEL" = stable ]; then
    case "$VERSION" in *-*) die "pre-release version $VERSION can't be published on the stable channel" ;; esac
fi
[ -s "$KEYRING" ] || die "trusted CA $KEYRING missing (release/keys/create-ota-keys.sh)"

if [ -z "$RAUC" ]; then
    if [ -x "$IMAGES/../host/bin/rauc" ]; then RAUC="$IMAGES/../host/bin/rauc"; else RAUC="$(command -v rauc || true)"; fi
fi
[ -n "$RAUC" ] && [ -x "$RAUC" ] || die "rauc not found (enable BR2_PACKAGE_HOST_RAUC or install rauc)"

WORK="$(mktemp -d)"
cleanup() { rm -rf "$WORK"; }
trap cleanup EXIT
umask 077
if [ -z "$CERT" ] && [ -n "${CUSTOMOS_SIGNING_CERT:-}" ]; then
    CERT="$WORK/signing.cert.pem"; printf '%s\n' "$CUSTOMOS_SIGNING_CERT" > "$CERT"
fi
if [ -z "$KEY" ] && [ -n "${CUSTOMOS_SIGNING_KEY:-}" ]; then
    KEY="$WORK/signing.key.pem"; printf '%s\n' "$CUSTOMOS_SIGNING_KEY" > "$KEY"
fi
[ -n "$CERT" ] && [ -s "$CERT" ] || die "no signing certificate (--cert, CUSTOMOS_SIGNING_CERT_FILE or CUSTOMOS_SIGNING_CERT)"
[ -n "$KEY" ] && [ -s "$KEY" ] || die "no signing key (--key, CUSTOMOS_SIGNING_KEY_FILE or CUSTOMOS_SIGNING_KEY)"
umask 022

OUT="${OUT:-$REPO/release/out/v$VERSION}"
mkdir -p "$OUT"
BUNDLE="CustomOS-v$VERSION.raucb"
MANIFEST="CustomOS-v$VERSION-manifest.json"
ISO="CustomOS-v$VERSION-x86_64.iso"
rm -f "$OUT/$BUNDLE" "$OUT/$MANIFEST" "$OUT/$ISO" "$OUT/SHA256SUMS" "$OUT/RELEASE_NOTES.md"

# ---- release notes ------------------------------------------------------------------
# The "## X.Y.Z ..." section of CHANGELOG.md, without its heading.
NOTES="$(awk -v v="$VERSION" '
    /^## / { if (found) exit; split($0, w, " "); if (w[2] == v) { found = 1; next } }
    found { print }' "$CHANGELOG" | sed -e '/./,$!d')"
[ -n "$NOTES" ] || die "$(basename "$CHANGELOG") has no section \"## $VERSION\""
printf '%s\n' "$NOTES" > "$OUT/RELEASE_NOTES.md"

# ---- RAUC bundle ------------------------------------------------------------------------
echo ">>> Creating $BUNDLE"
mkdir -p "$WORK/bundle"
ln "$IMAGES/rootfs.squashfs" "$WORK/bundle/rootfs.squashfs" 2>/dev/null || cp "$IMAGES/rootfs.squashfs" "$WORK/bundle/"
cat > "$WORK/bundle/manifest.raucm" <<EOF
[update]
compatible=$COMPATIBLE
version=$VERSION
build=${BUILD_ID:-unknown}
description=CustomOS $VERSION

[bundle]
format=verity

[image.rootfs]
filename=rootfs.squashfs
EOF
"$RAUC" bundle --cert="$CERT" --key="$KEY" "$WORK/bundle" "$OUT/$BUNDLE" >/dev/null

# The bundle must verify the way an installed system checks it: against the
# CA that the images trust, with the same keyring rules as rauc-system.conf.
echo ">>> Verifying the signature against $(basename "$KEYRING")"
cat > "$WORK/verify.conf" <<EOF
[system]
compatible=$COMPATIBLE
bootloader=noop
data-directory=$WORK/rauc-data

[keyring]
path=$KEYRING
check-purpose=codesign
use-bundle-signing-time=true
EOF
"$RAUC" --conf="$WORK/verify.conf" info --output-format=shell "$OUT/$BUNDLE" > "$WORK/info" 2>"$WORK/info.err" \
    || { cat "$WORK/info.err" >&2; die "the bundle does not verify against $KEYRING: wrong signing certificate?"; }
grep -q "^RAUC_MF_COMPATIBLE='$COMPATIBLE'$" "$WORK/info" || die "bundle compatible mismatch"
grep -q "^RAUC_MF_VERSION='$VERSION'$" "$WORK/info" || die "bundle version mismatch"

# ---- ISO ----------------------------------------------------------------------------------
ln "$IMAGES/CustomOS-x86_64.iso" "$OUT/$ISO" 2>/dev/null || cp "$IMAGES/CustomOS-x86_64.iso" "$OUT/$ISO"

# ---- manifest -----------------------------------------------------------------------------
BUNDLE_SIZE=$(stat -c %s "$OUT/$BUNDLE")
BUNDLE_SHA=$(sha256sum "$OUT/$BUNDLE" | cut -d' ' -f1)
GIT_COMMIT=$(git -C "$REPO" rev-parse HEAD 2>/dev/null || echo unknown)
M_VERSION="$VERSION" M_CHANNEL="$CHANNEL" M_COMPATIBLE="$COMPATIBLE" M_BUNDLE="$BUNDLE" \
M_SIZE="$BUNDLE_SIZE" M_SHA="$BUNDLE_SHA" M_NOTES="$OUT/RELEASE_NOTES.md" M_BUILD="${BUILD_ID:-}" \
M_COMMIT="$GIT_COMMIT" M_BUILDROOT="${BUILDROOT_VERSION:-2026.02.3}" M_MIN="$MIN_VERSION" \
python3 - "$OUT/$MANIFEST" <<'EOF'
import datetime, json, os, sys
e = os.environ
manifest = {
    "schema": 1,
    "product": "CustomOS",
    "version": e["M_VERSION"],
    "channel": e["M_CHANNEL"],
    "compatible": e["M_COMPATIBLE"],
    "released": datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
    "reboot_required": True,
    "bundle": {"name": e["M_BUNDLE"], "size": int(e["M_SIZE"]), "sha256": e["M_SHA"]},
    "changelog": open(e["M_NOTES"], encoding="utf-8").read().strip(),
    "build": {"id": e["M_BUILD"], "git_commit": e["M_COMMIT"], "buildroot": e["M_BUILDROOT"]},
}
if e["M_MIN"]:
    manifest["min_version"] = e["M_MIN"]
with open(sys.argv[1], "w", encoding="utf-8") as f:
    json.dump(manifest, f, indent=2, ensure_ascii=False)
    f.write("\n")
EOF

( cd "$OUT" && sha256sum "$BUNDLE" "$MANIFEST" "$ISO" > SHA256SUMS )

echo
echo "Release files for CustomOS $VERSION in $OUT:"
( cd "$OUT" && ls -l "$BUNDLE" "$MANIFEST" "$ISO" SHA256SUMS RELEASE_NOTES.md )
echo "Publish them with: release/publish-github-release.sh $OUT"

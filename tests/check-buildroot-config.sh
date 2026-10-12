#!/bin/bash
# check-buildroot-config.sh: static checks of the Buildroot configuration that
# need no source downloads (CI runs this on every push).
#
#   tests/check-buildroot-config.sh [BUILDROOT_DIR]
#
#  * both defconfigs resolve without Kconfig warnings and keep every option
#    they ask for;
#  * customos_x86_64_ota_defconfig contains customos_x86_64_defconfig
#    (apart from the lines marked "OTA:");
#  * all package recipes parse (make show-info);
#  * Buildroot's check-package finds no errors in the external tree.
# Must not run as root (Buildroot refuses some steps as root).
set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
BR_VERSION=$(sed -n 's/^BUILDROOT_VERSION="\(.*\)"/\1/p' "$REPO/build.sh")
BR="${1:-${BUILDROOT_DIR:-$REPO/buildroot}}"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
fail=0
err() { echo "FAIL: $*"; fail=1; }

if [ ! -f "$BR/Makefile" ]; then
    git clone -q --depth 1 --branch "$BR_VERSION" https://gitlab.com/buildroot.org/buildroot.git "$BR"
fi

for cfg in customos_x86_64_defconfig customos_x86_64_ota_defconfig; do
    out="$WORK/$cfg"
    log=$(make -s -C "$BR" O="$out" BR2_EXTERNAL="$REPO/br2-external" "$cfg" 2>&1)
    if echo "$log" | grep -qiE 'warning|error|recursive'; then
        err "$cfg: Kconfig reported problems:"
        echo "$log" | grep -iE 'warning|error|recursive' | sed 's/^/    /'
    fi
    while IFS= read -r line; do
        key=${line%%=*}
        got=$(grep -E "^$key=" "$out/.config" | head -n1 || true)
        [ "$got" = "$line" ] || err "$cfg: '$line' became '${got:-<not set>}'"
    done < <(grep -E '^BR2_' "$REPO/br2-external/configs/$cfg")
    make -s -C "$out" show-info > "$out/show-info.json" 2> "$out/show-info.err" || {
        err "$cfg: make show-info failed"; cat "$out/show-info.err"; }
    echo "ok: $cfg ($(python3 -c 'import json,sys; print(len(json.load(open(sys.argv[1]))))' "$out/show-info.json") packages)"
done

# Every line of the base defconfig is in the OTA defconfig, except the ones the
# OTA edition overrides (each preceded by an "# OTA:" comment).
python3 - "$REPO/br2-external/configs" <<'EOF' || fail=1
import sys
d = sys.argv[1]
base = [l.strip() for l in open(f"{d}/customos_x86_64_defconfig") if l.startswith("BR2_")]
ota_lines = open(f"{d}/customos_x86_64_ota_defconfig").read().splitlines()
overridden = {ota_lines[i + 1].split("=")[0] for i, l in enumerate(ota_lines[:-1]) if l.startswith("# OTA:")}
ota = {l.strip() for l in ota_lines if l.startswith("BR2_")}
missing = [l for l in base if l not in ota and l.split("=")[0] not in overridden]
for l in missing:
    print(f"FAIL: customos_x86_64_ota_defconfig lacks '{l}' from customos_x86_64_defconfig")
sys.exit(1 if missing else 0)
EOF

( cd "$REPO/br2-external" && "$BR/utils/check-package" --br2-external package/*/* board/customos/*.sh board/customos/ota/* configs/* ) \
    > "$WORK/check-package.log" 2>&1 || true
if grep -vE 'missing BR2_DOWNLOAD_FORCE_CHECK_HASHES|lines processed|warnings generated|help text: <tab><2 spaces>' "$WORK/check-package.log" | grep -q .; then
    err "check-package:"
    cat "$WORK/check-package.log"
else
    echo "ok: check-package ($(tail -n1 "$WORK/check-package.log"))"
fi
exit "$fail"

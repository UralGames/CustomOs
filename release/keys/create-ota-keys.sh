#!/bin/bash
# create-ota-keys.sh: create the keys that sign CustomOS update bundles.
#
#   release/keys/create-ota-keys.sh [--dir DIR] [--no-passphrase] [--signing-only] [--no-install]
#
# Creates, in DIR (default: ~/.customos-signing, outside the repository):
#   ca.key.pem         root CA private key       OFFLINE: never upload it anywhere
#   ca.cert.pem        root CA certificate        public: the trust anchor in every image
#   signing.key.pem    release signing key        secret: GitHub secret CUSTOMOS_SIGNING_KEY
#   signing.cert.pem   release signing cert       GitHub secret CUSTOMOS_SIGNING_CERT
# and copies ca.cert.pem to keys/customos-ota-ca.cert.pem in the repository
# (safe to commit; the image build installs it as /etc/rauc/keyring.pem).
#
# The root CA key is encrypted with a passphrase you choose (unless
# --no-passphrase, meant for throw-away test keys). With --signing-only an
# existing CA in DIR signs a new release signing key (key rotation).
# --no-install leaves keys/ in the repository alone (test keys).
# See docs/SIGNING.md.
set -euo pipefail

DIR="$HOME/.customos-signing"
PASS=1
SIGNING_ONLY=0
INSTALL=1
while [ $# -gt 0 ]; do
    case "$1" in
        --dir) DIR="$2"; shift 2 ;;
        --no-passphrase) PASS=0; shift ;;
        --signing-only) SIGNING_ONLY=1; shift ;;
        --no-install) INSTALL=0; shift ;;
        -h|--help) sed -n '2,23p' "$0"; exit 0 ;;
        *) echo "create-ota-keys: unknown option $1" >&2; exit 2 ;;
    esac
done

REPO="$(cd "$(dirname "$0")/../.." && pwd)"
case "$(realpath -m "$DIR")/" in
    "$REPO"/*) echo "create-ota-keys: $DIR is inside the repository; keep private keys elsewhere." >&2; exit 1 ;;
esac
command -v openssl >/dev/null || { echo "create-ota-keys: openssl is required" >&2; exit 1; }

umask 077
mkdir -p "$DIR"
chmod 700 "$DIR"
cd "$DIR"

if [ "$SIGNING_ONLY" -eq 0 ]; then
    if [ -e ca.key.pem ]; then
        echo "create-ota-keys: $DIR/ca.key.pem already exists; use --signing-only to rotate the signing key." >&2
        exit 1
    fi
    echo ">>> Creating the CustomOS OTA root CA (valid 20 years)"
    if [ "$PASS" -eq 1 ]; then
        openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:P-256 -aes-256-cbc -out ca.key.pem
    else
        openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:P-256 -out ca.key.pem
    fi
    openssl req -x509 -new -key ca.key.pem -sha256 -days 7305 \
        -subj "/O=CustomOS/CN=CustomOS OTA Root CA" \
        -addext "basicConstraints=critical,CA:TRUE,pathlen:0" \
        -addext "keyUsage=critical,keyCertSign,cRLSign" \
        -addext "subjectKeyIdentifier=hash" \
        -out ca.cert.pem
else
    [ -e ca.key.pem ] && [ -e ca.cert.pem ] || { echo "create-ota-keys: no CA in $DIR" >&2; exit 1; }
    stamp=$(date -u +%Y%m%d%H%M%S)
    for f in signing.key.pem signing.cert.pem; do
        [ -e "$f" ] && mv "$f" "$f.$stamp.old"
    done
fi

echo ">>> Creating the release signing key and certificate (valid 5 years)"
openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:P-256 -out signing.key.pem
openssl req -new -key signing.key.pem -subj "/O=CustomOS/CN=CustomOS OTA Release Signing $(date -u +%Y-%m)" \
    -out signing.csr.pem
cat > signing.ext <<'EOF'
basicConstraints=critical,CA:FALSE
keyUsage=critical,digitalSignature
extendedKeyUsage=critical,codeSigning
subjectKeyIdentifier=hash
authorityKeyIdentifier=keyid,issuer
EOF
openssl x509 -req -in signing.csr.pem -CA ca.cert.pem -CAkey ca.key.pem -CAcreateserial \
    -sha256 -days 1826 -extfile signing.ext -out signing.cert.pem
rm -f signing.csr.pem signing.ext
openssl verify -purpose any -CAfile ca.cert.pem signing.cert.pem >/dev/null

if [ "$INSTALL" -eq 1 ]; then
    mkdir -p "$REPO/keys"
    cp ca.cert.pem "$REPO/keys/customos-ota-ca.cert.pem"
    chmod 0644 "$REPO/keys/customos-ota-ca.cert.pem"
fi

if [ "$INSTALL" -eq 1 ]; then
    ca_note="copied to keys/customos-ota-ca.cert.pem: commit that file"
else
    ca_note="public trust anchor (keys/ in the repository was left alone)"
fi
cat <<EOF

Done. Files in $DIR:
  ca.key.pem        OFFLINE root key: back it up (e.g. on two USB sticks), never upload it
  ca.cert.pem       $ca_note
  signing.key.pem   -> GitHub secret CUSTOMOS_SIGNING_KEY  (paste the whole PEM)
  signing.cert.pem  -> GitHub secret CUSTOMOS_SIGNING_CERT (paste the whole PEM)

With the GitHub CLI:
  gh secret set CUSTOMOS_SIGNING_KEY  < "$DIR/signing.key.pem"
  gh secret set CUSTOMOS_SIGNING_CERT < "$DIR/signing.cert.pem"

EOF
if [ "$SIGNING_ONLY" -eq 1 ]; then
    echo "The previous signing key and certificate were renamed to *.old. Installed"
    echo "systems accept bundles signed with the new key at once (same CA)."
else
    echo "Images built with keys/customos-ota-ca.cert.pem trust only this CA. Release"
    echo "bundles must be signed with signing.key.pem (release/make-release.sh does it)."
fi

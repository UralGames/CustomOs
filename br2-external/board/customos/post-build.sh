#!/bin/sh
# Buildroot post-build script: final adjustments of the target file system.
# $1 = TARGET_DIR
set -eu
TARGET_DIR="$1"

# ---- Identity -------------------------------------------------------------------
cat > "$TARGET_DIR/usr/lib/os-release" <<'EOF'
NAME="CustomOS"
PRETTY_NAME="CustomOS 1.0 (Meridian)"
ID=customos
VERSION="1.0 (Meridian)"
VERSION_ID=1.0
VERSION_CODENAME=meridian
HOME_URL="https://example.invalid/customos"
LOGO=customos-logo
EOF
ln -sf ../usr/lib/os-release "$TARGET_DIR/etc/os-release"
echo "customos" > "$TARGET_DIR/etc/hostname"

# ---- Permissions that the overlay cannot carry ------------------------------
chmod 0750 "$TARGET_DIR/etc/sudoers.d"
chmod 0440 "$TARGET_DIR/etc/sudoers.d/"*
if [ -d "$TARGET_DIR/etc/polkit-1/rules.d" ]; then
    chmod 0750 "$TARGET_DIR/etc/polkit-1/rules.d"
    chown 0:0 "$TARGET_DIR/etc/polkit-1/rules.d" 2>/dev/null || true
fi
chmod 0755 "$TARGET_DIR/etc/profile.d/customos.sh"

# ---- Password hashing -------------------------------------------------------------
# shadow's chpasswd/useradd use SHA-512 like the build-time users table.
if [ -f "$TARGET_DIR/etc/login.defs" ]; then
    sed -i -e 's/^#\?\s*ENCRYPT_METHOD.*/ENCRYPT_METHOD SHA512/' "$TARGET_DIR/etc/login.defs"
    grep -q '^ENCRYPT_METHOD' "$TARGET_DIR/etc/login.defs" || echo 'ENCRYPT_METHOD SHA512' >> "$TARGET_DIR/etc/login.defs"
fi

# ---- PipeWire / WirePlumber in every user session -----------------------------
user_wants() {   # user_wants <target> <unit>
    unit_path=""
    for d in usr/lib/systemd/user lib/systemd/user; do
        [ -e "$TARGET_DIR/$d/$2" ] && { unit_path="/$d/$2"; break; }
    done
    [ -n "$unit_path" ] || return 0
    mkdir -p "$TARGET_DIR/etc/systemd/user/$1.wants"
    ln -sf "$unit_path" "$TARGET_DIR/etc/systemd/user/$1.wants/$2"
}
user_wants sockets.target pipewire.socket
user_wants sockets.target pipewire-pulse.socket
user_wants default.target pipewire.service
user_wants default.target wireplumber.service
if [ -e "$TARGET_DIR/usr/lib/systemd/user/wireplumber.service" ]; then
    ln -sf /usr/lib/systemd/user/wireplumber.service \
        "$TARGET_DIR/etc/systemd/user/pipewire-session-manager.service"
fi

# ---- Desktop defaults ------------------------------------------------------------
# Adwaita cursors for the compositor and Qt.
mkdir -p "$TARGET_DIR/usr/share/icons/default"
cat > "$TARGET_DIR/usr/share/icons/default/index.theme" <<'EOF'
[Icon Theme]
Inherits=Adwaita
EOF

# The live user's home is created from /etc/skel at build time by Buildroot's
# users table; make sure the standard folders exist there too.
if [ -d "$TARGET_DIR/home/live" ]; then
    for d in Desktop Documents Downloads Music Pictures Videos; do
        mkdir -p "$TARGET_DIR/home/live/$d"
    done
fi

# No serial/agetty on tty1: the graphical sign-in owns it.
rm -f "$TARGET_DIR/etc/systemd/system/getty.target.wants/getty@tty1.service"

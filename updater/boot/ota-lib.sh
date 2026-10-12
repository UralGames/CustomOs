# shellcheck shell=bash
# CustomOS A/B helpers shared by customos-preinit (early boot) and
# customos-install-ab (the installer). Sourced, never executed.
#
# Installed-system layout (GPT):
#   1  bios-boot          1 MiB   GRUB core image for legacy BIOS
#   2  customos-esp     512 MiB   FAT32: GRUB (BIOS + UEFI), grub.cfg, grubenv
#   3  customos-rootfs-a          slot A: read-only squashfs root file system
#   4  customos-rootfs-b          slot B: read-only squashfs root file system
#   5  customos-data        rest  ext4: /home, settings, update state
#
# The root file system never changes while it runs. Everything that must
# survive an update lives on the data partition and is overlaid at boot:
#   /etc  overlay, upper = /data/overlay/etc/upper  (only the persistent-etc.list
#         entries survive a version change; the rest is reset to the new defaults)
#   /var  overlay, upper = /data/overlay/var/upper
#   /root overlay, upper = /data/overlay/root/upper
#   /home bind mount of /data/home

OTA_LIBDIR=${OTA_LIBDIR:-/usr/lib/customos}
OTA_DATA_LABEL=customos-data
OTA_PART_ESP=2
OTA_PART_A=3
OTA_PART_B=4
OTA_PART_DATA=5
# First UID/GID of accounts people create (installer, useradd). Buildroot gives
# every account baked into the image an id below 1000.
OTA_FIRST_LOCAL_ID=1000
OTA_LAST_LOCAL_ID=59999
# Accounts that exist only on the live image.
OTA_LIVE_ONLY_USERS="live"

ota_log() { echo "customos: $*" >&2; }

# ota_partition_dev DISK N -> /dev/sda3, /dev/nvme0n1p3, /dev/mmcblk0p3, /dev/loop0p3
ota_partition_dev() {
    case "$1" in
        *[0-9]) echo "${1}p$2" ;;
        *) echo "$1$2" ;;
    esac
}

# ota_parent_disk /dev/sda3 -> /dev/sda (via sysfs)
ota_parent_disk() {
    local name sys
    name=$(basename "$(readlink -f "$1")")
    sys=$(readlink -f "/sys/class/block/$name") || return 1
    [ -e "$sys/partition" ] || return 1
    echo "/dev/$(basename "$(dirname "$sys")")"
}

# ota_find_partuuid UUID -> device node (no udev needed: blkid reads /proc/partitions)
ota_find_partuuid() {
    local uuid=$1 dev i
    for i in $(seq 1 100); do
        dev=$(blkid -l -t "PARTUUID=$uuid" -o device 2>/dev/null)
        if [ -n "$dev" ] && [ -b "$dev" ]; then
            echo "$dev"
            return 0
        fi
        [ "$i" -eq 1 ] && ota_log "waiting for partition $uuid"
        sleep 0.3
    done
    return 1
}

# ota_prepare_data DATADIR: create the directory layout on a mounted data partition.
ota_prepare_data() {
    local d=$1 x
    for x in etc var root; do
        mkdir -p "$d/overlay/$x/upper" "$d/overlay/$x/work"
    done
    mkdir -p "$d/home" "$d/customos/update" "$d/customos/etc-backup" "$d/rauc"
    chmod 0755 "$d" "$d/overlay" "$d/home"
    chmod 0700 "$d/customos/update" "$d/customos/etc-backup" "$d/rauc"
}

# ota_mount_overlay ROOT DIR DATADIR: overlay ROOT/DIR with the persistent upper layer.
ota_mount_overlay() {
    local root=$1 dir=$2 data=$3
    mount -t overlay "overlay-$dir" \
        -o "lowerdir=$root/$dir,upperdir=$data/overlay/$dir/upper,workdir=$data/overlay/$dir/work" \
        "$root/$dir"
}

# ota_mount_tree ROOT DATADIR: the runtime view of an installed system rooted at
# ROOT (ROOT is "" at boot, the target mount point in the installer).
# ROOT/data must already be the mounted data partition.
ota_mount_tree() {
    local root=$1 data=$2 x
    for x in etc var root; do
        ota_mount_overlay "$root" "$x" "$data" || return 1
    done
    mount --bind "$data/home" "$root/home" || return 1
}

# ---- /etc across version changes -------------------------------------------------

# ota_etc_keep REL: is /etc/REL listed in persistent-etc.list (or an ancestor of an entry)?
ota_etc_keep() {
    local rel=$1 entry
    while IFS= read -r entry; do
        case "$entry" in ''|'#'*) continue ;; esac
        if [ "${entry%/}" != "$entry" ]; then        # directory entry "dir/"
            [ "$rel" = "${entry%/}" ] && return 0
            case "$rel" in "$entry"*) return 0 ;; esac
        else
            [ "$rel" = "$entry" ] && return 0
        fi
        # ancestors of an entry are kept as directories
        case "$entry" in "$rel"/*) [ -d "$OTA_UPPER/$rel" ] && [ ! -L "$OTA_UPPER/$rel" ] && return 0 ;; esac
    done < "$OTA_LIBDIR/persistent-etc.list"
    return 1
}

# ota_prune_etc UPPER BACKUPDIR: after a version change, move every /etc
# customisation that is not a supported persistent setting out of the overlay
# (into BACKUPDIR), so the new version's defaults apply.
ota_prune_etc() {
    OTA_UPPER=$1
    local backup=$2 path rel moved=0
    [ -d "$OTA_UPPER" ] || return 0
    while IFS= read -r -d '' path; do
        rel=${path#"$OTA_UPPER"/}
        ota_etc_keep "$rel" && continue
        if [ -d "$path" ] && [ ! -L "$path" ]; then
            rmdir "$path" 2>/dev/null || true    # emptied by the depth-first walk
            continue
        fi
        mkdir -p "$backup/$(dirname "$rel")"
        mv -f "$path" "$backup/$rel" && moved=$((moved + 1))
    done < <(find "$OTA_UPPER" -mindepth 1 -depth -print0)
    [ "$moved" -gt 0 ] && ota_log "reset $moved /etc file(s) to the new version's defaults (backup: $backup)"
    return 0
}

# ---- accounts -------------------------------------------------------------------

_ota_is_live_user() {
    local u
    for u in $OTA_LIVE_ONLY_USERS; do [ "$1" = "$u" ] && return 0; done
    return 1
}

_ota_is_local_id() {
    case "$1" in ''|*[!0-9]*) return 1 ;; esac
    [ "$1" -ge "$OTA_FIRST_LOCAL_ID" ] && [ "$1" -le "$OTA_LAST_LOCAL_ID" ]
}

# ota_merge_accounts LOWER CURRENT OUT: build the account database of an
# installed system. LOWER holds the image's passwd/group/shadow/gshadow,
# CURRENT the persistent ones (may be missing), OUT receives the result.
#  * accounts of the image (uid < 1000) come from the image, so updates can
#    add or change system users; their shadow entries (root's password lock)
#    are kept from CURRENT;
#  * accounts people created (uid >= 1000) come from CURRENT;
#  * group memberships of image groups are the union of both, so a user stays
#    in wheel, audio, video and so on;
#  * live-only accounts (the "live" user) are dropped.
ota_merge_accounts() {
    local lower=$1 cur=$2 out=$3
    local name pw uid gid rest line members
    declare -A lowerUser=() lowerGroup=() curShadow=() curGshadow=() curMembers=() keptUser=() lowerGroupGid=()
    mkdir -p "$out"

    # -- passwd
    : > "$out/passwd"
    while IFS=: read -r name pw uid gid rest; do
        [ -n "$name" ] || continue
        _ota_is_live_user "$name" && continue
        lowerUser[$name]=1
        keptUser[$name]=1
        printf '%s:%s:%s:%s:%s\n' "$name" "$pw" "$uid" "$gid" "$rest" >> "$out/passwd"
    done < "$lower/passwd"
    if [ -f "$cur/passwd" ]; then
        while IFS=: read -r name pw uid gid rest; do
            [ -n "$name" ] || continue
            [ -n "${lowerUser[$name]:-}" ] && continue
            _ota_is_live_user "$name" && continue
            _ota_is_local_id "$uid" || continue
            keptUser[$name]=1
            printf '%s:%s:%s:%s:%s\n' "$name" "$pw" "$uid" "$gid" "$rest" >> "$out/passwd"
        done < "$cur/passwd"
    fi

    # -- shadow
    if [ -f "$cur/shadow" ]; then
        while IFS= read -r line; do
            curShadow[${line%%:*}]=$line
        done < "$cur/shadow"
    fi
    : > "$out/shadow"
    while IFS= read -r line; do
        name=${line%%:*}
        [ -n "$name" ] && [ -n "${keptUser[$name]:-}" ] || continue
        printf '%s\n' "${curShadow[$name]:-$line}" >> "$out/shadow"
        unset "curShadow[$name]"
    done < "$lower/shadow"
    for name in "${!curShadow[@]}"; do
        [ -n "${keptUser[$name]:-}" ] && [ -z "${lowerUser[$name]:-}" ] && printf '%s\n' "${curShadow[$name]}" >> "$out/shadow"
    done

    # -- group
    if [ -f "$cur/group" ]; then
        while IFS=: read -r name pw gid members; do
            [ -n "$name" ] && curMembers[$name]=$members
        done < "$cur/group"
    fi
    : > "$out/group"
    while IFS=: read -r name pw gid members; do
        [ -n "$name" ] || continue
        _ota_is_live_user "$name" && continue
        lowerGroup[$name]=1
        lowerGroupGid[$name]=$gid
        members=$(_ota_union_members "$members" "${curMembers[$name]:-}")
        printf '%s:%s:%s:%s\n' "$name" "$pw" "$gid" "$members" >> "$out/group"
    done < "$lower/group"
    if [ -f "$cur/group" ]; then
        while IFS=: read -r name pw gid members; do
            [ -n "$name" ] || continue
            [ -n "${lowerGroup[$name]:-}" ] && continue
            _ota_is_live_user "$name" && continue
            _ota_is_local_id "$gid" || continue
            printf '%s:%s:%s:%s\n' "$name" "$pw" "$gid" "$(_ota_union_members "$members" "")" >> "$out/group"
        done < "$cur/group"
    fi

    # -- gshadow (optional on Buildroot images)
    if [ -f "$lower/gshadow" ] || [ -f "$cur/gshadow" ]; then
        if [ -f "$cur/gshadow" ]; then
            while IFS= read -r line; do curGshadow[${line%%:*}]=$line; done < "$cur/gshadow"
        fi
        : > "$out/gshadow"
        while IFS=: read -r name _ _ _; do
            [ -n "$name" ] || continue
            if [ -n "${curGshadow[$name]:-}" ]; then
                printf '%s\n' "${curGshadow[$name]}" >> "$out/gshadow"
            elif [ -f "$lower/gshadow" ] && line=$(grep -m1 "^$name:" "$lower/gshadow"); then
                printf '%s\n' "$line" >> "$out/gshadow"
            else
                printf '%s:!::\n' "$name" >> "$out/gshadow"
            fi
        done < "$out/group"
    fi
}

# Comma-separated union of two member lists, without live-only users and
# without users that no longer exist (keptUser from the caller).
_ota_union_members() {
    local all=$1,$2 m out="" seen=","
    local IFS=,
    for m in $all; do
        [ -n "$m" ] || continue
        _ota_is_live_user "$m" && continue
        [ -n "${keptUser[$m]:-}" ] || continue
        case "$seen" in *",$m,"*) continue ;; esac
        seen="$seen$m,"
        out=${out:+$out,}$m
    done
    printf '%s' "$out"
}

# ota_install_accounts DIR ETC: atomically replace ETC/{passwd,group,shadow,gshadow}.
ota_install_accounts() {
    local src=$1 etc=$2 f mode
    for f in passwd group shadow gshadow; do
        [ -f "$src/$f" ] || continue
        case "$f" in shadow|gshadow) mode=0600 ;; *) mode=0644 ;; esac
        if [ -f "$etc/$f" ] && cmp -s "$src/$f" "$etc/$f"; then
            continue
        fi
        install -m "$mode" "$src/$f" "$etc/.$f.customos-new" && mv -f "$etc/.$f.customos-new" "$etc/$f"
    done
}

# ota_installed_mode ETC: remove what makes the image a live system.
ota_installed_mode() {
    local etc=$1
    rm -f "$etc/customos-live" "$etc/sudoers.d/live"
    if [ -f "$etc/meridian/login.conf" ] && grep -q '^[[:space:]]*autologin' "$etc/meridian/login.conf"; then
        sed -i '/^[[:space:]]*autologin/d' "$etc/meridian/login.conf"
    fi
}

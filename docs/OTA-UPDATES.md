# Over-the-air updates

CustomOS updates itself from **GitHub Releases**. GitHub is only the
distribution server: what gets installed is a **RAUC bundle** that must carry
a valid signature from the CustomOS signing key, and it is written into the
**inactive half of an A/B disk layout**, never over the running system. If
the new version doesn't start, the computer goes back to the previous one by
itself.

This document describes how it works. Related documents:
- [SIGNING.md](SIGNING.md): keys, GitHub secrets, rotation.
- [RELEASING.md](RELEASING.md): how to publish a version.
- [VERSIONING.md](VERSIONING.md): version numbers and channels.
- [MIGRATION.md](MIGRATION.md): moving from CustomOS 0.1 and from an existing build.

## Contents of a release

A GitHub release `vX.Y.Z` carries four files, produced by `release/make-release.sh`:

| Asset | Contents |
|---|---|
| `CustomOS-vX.Y.Z.raucb` | Signed RAUC bundle ("verity" format) with the system image (`rootfs.squashfs`) |
| `CustomOS-vX.Y.Z-manifest.json` | Version, compatible string, bundle size and SHA-256, changelog, build information |
| `CustomOS-vX.Y.Z-x86_64.iso` | Live/installer ISO of the same version |
| `SHA256SUMS` | SHA-256 of the three files above |

Example manifest:

```json
{
  "schema": 1,
  "product": "CustomOS",
  "version": "0.3.0",
  "channel": "stable",
  "compatible": "customos-x86_64-ab",
  "released": "2026-10-12T09:00:00Z",
  "reboot_required": true,
  "bundle": { "name": "CustomOS-v0.3.0.raucb", "size": 431234567, "sha256": "…" },
  "changelog": "…Markdown from CHANGELOG.md…",
  "build": { "id": "3f9c2a1b7e4d", "git_commit": "…", "buildroot": "2026.02.3" }
}
```

An optional `"min_version"` lets a release refuse direct updates from versions
that are too old.

## What happens on the computer

```
GitHub Releases ──HTTPS──> customos-updated (root, D-Bus org.customos.Update1)
                             │ check → download → verify → install
                             ▼
                           RAUC ──writes──> inactive slot (A or B)
                             │                 ▲
                             └─ grubenv ───────┘  GRUB boots it; customos-health-check
                                                  confirms it or rolls it back
Settings › System › Updates / Meridian Update / customos-update  ──D-Bus──┘
```

1. **Check.** `customos-updated` asks GitHub for the newest release of the
   configured repository (`/repos/OWNER/NAME/releases/latest`, which only
   returns releases that are not pre-releases). It runs at start-up, every
   six hours, and when you press *Check for updates*. If the release is newer
   than the running version (Semantic Versioning order) it downloads the small
   manifest and `SHA256SUMS`, and requires all of the following before it
   offers the update:
   - the manifest is valid and its version equals the tag;
   - its `compatible` equals this system's (`customos-x86_64-ab`);
   - the bundle named in the manifest is attached, with the same size;
   - `SHA256SUMS` and GitHub's own asset digest list the same SHA-256 as the manifest;
   - the running version is at least `min_version`, if one is given.
2. **Download.** The bundle goes to `/data/customos/update/cache/`, after a
   free-space check. An interrupted download resumes where it stopped (HTTP
   Range). Redirects to GitHub's storage host are followed without the
   GitHub token.
3. **Verify.**
   - **Integrity:** the SHA-256 of the downloaded file must match. A damaged
     file is deleted.
   - **Authenticity:** RAUC checks the bundle's CMS signature against
     `/etc/rauc/keyring.pem`, the CustomOS CA. The signing certificate must be
     issued by that CA and carry the *code signing* key usage.
   - **Consistency:** the *signed* manifest inside the bundle must name the
     same compatible and version as the release, and that version must be
     newer than the running one. This blocks downgrades: an attacker who can
     edit releases could otherwise re-publish an old, genuinely signed bundle
     with known bugs.
   SHA-256 alone proves nothing about who made the file; only the signature does.
4. **Install.** RAUC writes the image into the **inactive** slot. The running
   system is not touched. RAUC's pre-install hook repeats the downgrade check
   for every installation path, including a manual `rauc install`. When the
   image is written, RAUC makes the new slot the first in the boot order. The
   post-install hook records the slot's version for the boot menu and marks
   the next boot as a *trial*.
5. **Restart.** Settings shows *Restart to finish the update*. Nothing
   restarts without you.
6. **Trial boot.** GRUB starts the new slot. `customos-health-check` waits
   for the services in `/etc/customos/health.conf` (D-Bus, RAUC, the update
   service and the desktop's sign-in service) and makes sure they keep
   running. If they do, the slot is confirmed (`rauc status mark-good`).
   Otherwise it is marked bad and the computer restarts into the previous
   version.
7. **Result.** After the restart the update service compares the slot that
   actually booted with the one it installed. It reads the health check's
   verdict from `/data/customos/update/last-trial.env` (kept until the next
   trial boot) and records the outcome in the history: either *Installed*,
   or *Rolled back* with the reason. The health check also writes the reason
   to the kernel log before the automatic restart. It
   appears on screen only when the kernel command line lacks `quiet`.

## A/B disk layout (installed systems)

`customos-install-ab` (used by the graphical installer of the OTA edition)
partitions the disk like this:

| # | GPT name | Size | Contents |
|---|---|---|---|
| 1 | `bios-boot` | 1 MiB | GRUB core image for legacy BIOS |
| 2 | `customos-esp` | 512 MiB | FAT32: GRUB for UEFI and BIOS, `grub/grub.cfg` (A/B menu), `grub/grubenv` |
| 3 | `customos-rootfs-a` | 2/4/6 GiB* | Slot A: read-only squashfs system image, kernel included |
| 4 | `customos-rootfs-b` | 2/4/6 GiB* | Slot B: the same, initially empty |
| 5 | `customos-data` | rest | ext4: `/home`, settings, update state |

\* 2 GiB on disks under 32 GB, 4 GiB from 32 GB, 6 GiB from 128 GB. The
system image (compressed) must fit in three quarters of a slot.

The system image is the same `rootfs.squashfs` that bundles carry. The
installer writes it to slot A byte for byte and reads it back to verify it.

### What survives an update

The system slot is read-only. Everything that changes at run time lives on
the data partition; `customos-preinit` (the first process of an installed
system) mounts it before systemd starts:

| Path | Storage |
|---|---|
| `/home` | bind mount of `/data/home` |
| `/etc` | overlay; changes in `/data/overlay/etc/upper` |
| `/var` | overlay; changes in `/data/overlay/var/upper` (logs, NetworkManager state, …) |
| `/root` | overlay; changes in `/data/overlay/root/upper` |
| `/media`, `/mnt` | RAM (mount points) |

When a new version boots for the first time, only the **supported persistent
settings** listed in [`/usr/lib/customos/persistent-etc.list`](../updater/boot/persistent-etc.list)
stay in `/etc`:
- accounts and passwords;
- computer name, time zone, locale;
- network connections;
- sign-in settings;
- sudo rules and SSH host keys;
- locally enabled services;
- update settings.

Other `/etc` edits are moved to `/data/customos/etc-backup/<old version>-<date>/`
(the three newest are kept) so the new version's defaults apply. Desktop
settings live in each user's home and are always kept.

Accounts are merged at every boot:
- accounts that come with the image (UID below 1000, created by Buildroot)
  come from the new version;
- accounts people created (UID 1000 and up) come from the data partition;
- the members of image groups (wheel, audio, video, …) are the union of both.

The live session's `live` user, its autologin and its sudo rule are removed
on installed systems.

### Boot loader and rollback

GRUB and its configuration are installed once by the installer. Updates never
modify them. The boot state is kept in `grubenv`, shared with RAUC's GRUB
back end:

| Variable | Meaning |
|---|---|
| `ORDER` | Slots by priority, e.g. `B A` after an update to B |
| `A_OK` / `B_OK` | 1 if the slot holds a usable system; RAUC sets 0 when marking it bad |
| `A_TRY` / `B_TRY` | Boots since the slot was last confirmed. GRUB counts up; the health check resets it to 0 |
| `A_VERSION` / `B_VERSION` | Shown in the boot menu |

GRUB boots the first slot in `ORDER` that is OK and has been tried fewer
than three times. A new version therefore gets three attempts:
- **Kernel panic:** `panic=10` restarts the machine.
- **Hang:** the hardware watchdog resets the machine (systemd
  `RuntimeWatchdogSec=30s`, when the hardware has one). Without a watchdog,
  you must switch the machine off and on.
- **Failed health check:** the slot is marked bad at once.

After that the previous slot starts again.

The boot menu also offers safe graphics and text console entries. *Previous
version* starts the other slot once, without changing the order.

**Settings › System › Updates › Go back** (or `customos-update rollback`)
makes the previous slot primary for good, when that slot still holds a
confirmed system. Files and settings are kept.

## Interfaces

**Settings › System › Updates** and **Meridian Update** (`meridian-update`)
show:
- the current version;
- *Check for updates*;
- available version, release notes and download size;
- download progress (with speed and time left) and installation progress;
- *Restart now*;
- the last successful update and the update history;
- clear error messages, with technical details on request;
- *Go back* to the previous version.

`meridian-update --notify`, started by the desktop session, announces new
versions, pending restarts and rollbacks.

Only administrators (members of `wheel`) can start downloads, installations,
restarts and rollbacks. Everyone can see the status.

**Command line** (`customos-update`):

```
customos-update                 # status
customos-update check
customos-update install [--yes] # download, verify, install
customos-update reboot
customos-update rollback [--yes]
customos-update history
customos-update status --json
```

**D-Bus** (`org.customos.Update1` at `/org/customos/Update1`, system bus):
- `GetStatus() → a{sv}` and `GetHistory() → s` (JSON);
- `CheckForUpdates()`, `Download()`, `Install()`, `Cancel()`, `Reboot()`,
  `Rollback()`, `DismissError()`;
- signals `StatusChanged(a{sv})` and `UpdateAvailable(ss)`.

## Configuration

`/etc/customos/update.conf` (defaults from the Buildroot configuration);
local changes go in `/etc/customos/update.conf.d/*.conf`, which survive updates:

```ini
[Source]
Repository=UralGames/CustomOs      ; owner/name or https://github.com/owner/name
ApiUrl=https://api.github.com      ; GitHub Enterprise: https://HOST/api/v3
Channel=stable                     ; "beta" is reserved
Compatible=customos-x86_64-ab
#TokenFile=/data/customos/update/github-token   ; only for a private repository

[Updates]
AutomaticCheck=true
CheckIntervalHours=6
StateDirectory=/data/customos/update
CacheDirectory=/data/customos/update/cache
AllowDowngrade=false
```

**Private repositories.** GitHub serves release files of a private
repository only with a token. Either:
- publish releases from a public repository (for example a separate
  `CustomOS-releases` repository; set `Repository` accordingly), or
- give each computer a read-only, fine-grained token with *Contents: read* on
  the repository, in a root-only file named by `TokenFile`.

A token in the image would be readable by everyone who downloads the ISO, so
never build one in.

## Security notes

- **The trust anchor** is `/etc/rauc/keyring.pem`, the public CA
  certificate. No private key is ever in an image. The CA's private key stays
  offline; CI holds only a release signing key issued by it.
- **Only verity bundles are accepted** (`bundle-formats=-plain`). Every block
  is authenticated while it is read during installation, so a bundle can't be
  swapped after its signature was checked.
- **Certificate validity** is checked at the bundle's signing time
  (`use-bundle-signing-time=true`), so computers with a wrong clock still
  accept genuine updates. If a signing key leaks, issue a new signing
  certificate and ship a keyring without the old CA, or with a CRL (see
  [SIGNING.md](SIGNING.md)).
- **RAUC's installer interface** is restricted to root on the system bus.
  RAUC's stock policy lets any user call it. The update service additionally
  checks that callers are administrators.
- **Not covered yet:**
  - Secure Boot: GRUB and the kernel are not signed; the ESP is trusted.
  - Encryption of the data partition.
  - Updating GRUB itself.

# CustomOS — Meridian desktop

CustomOS is a from-source x86_64 Linux system with its own Wayland desktop,
**Meridian**, written in Qt 6 / QML. It boots from an ISO into a live
session (signed in automatically), runs in VirtualBox and VMware, and can
optionally be installed to a disk.

> **Status — please read.** No ISO came out of the cloud environment this
> project was developed in. Its network policy blocks the hosts Buildroot
> downloads sources from. In a `make source` run, 178 of 225 packages failed
> to download. The blocked hosts include:
> - `sources.buildroot.net`, Buildroot's own mirror and the fallback for every package;
> - `toolchains.bootlin.com` (the toolchain);
> - `cdn.kernel.org`, `ftpmirror.gnu.org`, `download.qt.io`, `gitlab.freedesktop.org`, `archive.mesa3d.org`;
> - `github.com` source archives (labwc, swaylock);
> - and about 45 more.
>
> **`CustomOS-x86_64.iso` therefore does not exist yet.** You produce it by
> running the one-command build below on a machine with normal internet
> access, or in GitHub Actions (see [docs/RELEASING.md](docs/RELEASING.md)).
> Everything needed is in this repository. See *What was verified* for
> exactly what has and has not been run.

## Build the ISO (one command)

```sh
./build.sh            # on a Debian/Ubuntu/Fedora host
./build.sh --docker   # or inside a container: only Docker or Podman needed
```

The result is `./CustomOS-x86_64.iso` plus its `.sha256` checksum.

What to expect:
- The first build downloads about 3 GB and needs about 30 GB of disk.
- It takes roughly 1–3 hours on an 8-core machine. Qt, LLVM (for Mesa's software renderer) and the kernel take most of that time.
- Later builds reuse `output/` and ccache.

Other commands:

| Command | Purpose |
|---|---|
| `./build.sh --ota` | Build the edition with over-the-air updates (A/B layout) in `output-ota/`; see below |
| `./build.sh --menuconfig` | Change the Buildroot configuration, then build |
| `./build.sh --clean` | Delete `output/` |
| `JOBS=4 ./build.sh` | Limit parallelism |
| `make -C buildroot O=$PWD/output meridian-desktop-rebuild all` | Rebuild only the desktop after editing `desktop/` |

Host packages needed without Docker (Debian/Ubuntu):
`build-essential bc cpio file git libncurses-dev perl python3 rsync unzip wget xz-utils bzip2`.
Run the build as a normal user, not as root.

To run the ISO in a virtual machine, see **[docs/VM-SETUP.md](docs/VM-SETUP.md)**.

## Over-the-air updates

The OTA edition (`./build.sh --ota`, version 0.2.0 and later) updates itself
from this repository's GitHub Releases:
1. **Check and offer.** It checks for a new stable release and shows the version, release notes and
   download size in **Settings › System › Updates** (and the Meridian Update
   app).
2. **Download and verify.** It downloads the signed RAUC bundle and verifies its SHA-256 *and* its
   signature against the CustomOS CA certificate built into the image.
3. **Install.** It writes the bundle into the inactive half of an A/B disk layout.
4. **Restart and confirm.** It asks you to restart. GRUB boots the new version on trial, and a health check
   confirms it. A version that fails to start is rolled back automatically.
5. **Keep your data.** Files, accounts and supported settings live on a separate data partition
   and are kept.

| Document | Contents |
|---|---|
| [docs/OTA-UPDATES.md](docs/OTA-UPDATES.md) | How updates work: release files, verification, A/B layout, rollback, configuration |
| [docs/SIGNING.md](docs/SIGNING.md) | Creating the signing keys, GitHub Actions secrets, key rotation |
| [docs/RELEASING.md](docs/RELEASING.md) | Publishing a version (tag → GitHub Actions, or locally) |
| [docs/VERSIONING.md](docs/VERSIONING.md) | Version numbers, channels (stable; beta reserved) |
| [docs/MIGRATION.md](docs/MIGRATION.md) | Your existing build, the first OTA build, moving from CustomOS 0.1 |

Before the first OTA build, create the keys once:
`release/keys/create-ota-keys.sh` (private keys stay in `~/.customos-signing/`,
only the public certificate goes into `keys/`).

While this repository is private, installed systems can't see its releases,
and GitHub's free runners are probably too small for a full build. See
[docs/RELEASING.md](docs/RELEASING.md) for both.

**Live ISO vs. installed system.** The live session runs from read-only
media and is never updated; install it to a disk to receive updates.
CustomOS 0.1 installations use a single-partition layout and must be
reinstalled once to get the A/B layout; see
[docs/MIGRATION.md](docs/MIGRATION.md).

## Repository layout

```
build.sh                    one-command build (fetches Buildroot 2026.02.3 LTS)
docker/Dockerfile           container build environment
br2-external/               Buildroot external tree = the distribution definition
  configs/customos_x86_64_defconfig   package manifest + system configuration
  board/customos/
    linux.fragment          kernel options for VirtualBox, VMware, QEMU, real PCs
    grub.cfg                ISO boot menu (live / safe graphics / text / reboot / off)
    grub-efi-embedded.cfg   UEFI GRUB bootstrap for the ISO
    users.txt               live user
    rootfs-overlay/         /etc files: autologin, sudo, polkit, NetworkManager, presets
    post-build.sh           os-release, permissions, PipeWire user units
    post-image.sh           names the image CustomOS-x86_64.iso + checksum
    splash.png              boot splash (psplash)
  package/                  recipes for software Buildroot lacks:
    meridian-desktop        this project's desktop (built from ../desktop)
    labwc, libsfdo          compositor/window manager and its icon library
    layer-shell-qt, extra-cmake-modules   Qt layer-shell support for the shell
    swaylock, grim          screen lock and screenshots
desktop/                    the Meridian desktop (CMake, Qt 6.5+)
  core/ style/              shared library, services, QML controls style
  shell/                    taskbar, start menu, panels, notifications (layer-shell client)
  apps/files settings monitor editor
  login/                    graphical sign-in daemon + greeter (PAM, logind)
  installer/                graphical installer + privileged install script
  session/                  labwc config, themes, session script, .desktop files
  assets/                   generated icons, wallpapers, shadows; Inter + JetBrains Mono fonts
  apps/settings › System › Updates, update/ (Meridian Update app + QML module)
updater/                    update service customos-updated, CLI customos-update,
  boot/                     preinit, A/B GRUB menu, health check, RAUC config and hooks
  installer/                customos-install-ab (A/B layout installer)
br2-external/package/customos-updater   its Buildroot recipe
br2-external/configs/customos_x86_64_ota_defconfig   the OTA edition
release/                    signing keys, signed bundle + release files, GitHub publishing
keys/                       public update CA certificate (no private keys)
.github/workflows/          CI checks; release build and publishing
tests/                      Buildroot config check, QEMU end-to-end test, UI mock
VERSION, CHANGELOG.md       version and release notes
docs/                       VM setup, architecture, updates, signing, releasing
```

## How it is built, and why not hand-run LFS

The brief asks for LFS/BLFS principles: build everything from source, with a
minimal userspace you control. CustomOS follows those principles. The
mechanics are automated with **Buildroot**, the reproducible from-source
build system:

- A pinned cross toolchain (Bootlin glibc, gcc 14).
- The Linux kernel (latest stable, 6.19.x) with `linux.fragment`.
- systemd, Mesa, wlroots, Qt 6.9, PipeWire, NetworkManager and the rest, each compiled from its upstream tarball.
- All source versions are pinned. Buildroot's own packages are checksum-verified.

A literal hand-typed LFS build cannot be reproduced with one command. It
also cannot be checked line by line, and it would take days per iteration.
No binary distribution is used or rebranded.

The other deviation from the original plan concerns the compositor. A
compositor written on Qt's QtWaylandCompositor could not run the terminal
(foot): Qt's compositor implements only older versions of the seat and
data-device protocols. So window management is done by **labwc** (wlroots).
The whole visible desktop is a Qt/QML layer-shell client with its own design:
- wallpaper, taskbar, start menu, quick settings, calendar, notification centre, task view, OSD and tooltips;
- the applications, the sign-in screen and the installer.

labwc is configured by the shell, which writes its `rc.xml` from Settings and
reloads it live. It is themed with Meridian's own generated window theme.

## Features

The desktop follows the familiar Windows 11 layout. All artwork is original:
icons, wallpapers, window buttons and logo are generated by scripts in
`desktop/assets` and `desktop/session/themes`. No Microsoft assets are used.

| Area | What works |
|---|---|
| Taskbar | Centred or left alignment (Settings › Taskbar); pinned apps; running indicators; grouping; right-click menus (pin/unpin, close); hover tooltips |
| Start menu | Pinned grid, all-apps list, search; power menu (lock, sign out, restart, shut down) |
| Tray | Network, volume and clock buttons; notification badge |
| Calendar | Clock flyout with a month calendar; 12/24-hour and seconds options |
| Notifications | org.freedesktop.Notifications server; toasts; notification centre; Do Not Disturb |
| Quick settings | Wi-Fi, network, Do Not Disturb, dark mode, desktops and screenshot tiles; volume slider with output-device chooser; Wi-Fi network list; link to all settings |
| Network | NetworkManager: Wi-Fi scan/connect (with password), Ethernet status, enable/disable |
| Volume | PipeWire/WirePlumber: output volume and mute, device choice; volume keys with OSD |
| Display | Resolution and scale via wlr-randr, with a 15-second keep/revert prompt |
| Theme and wallpaper | Dark/light mode and accent colour, live across shell, apps and window borders; 6 wallpapers (including **tux-sunset**) plus import of your own |
| Windows | Minimise, maximise, close; snapping by drag and Super+←/→/↑/↓ |
| Window switching | Alt+Tab switcher with previews |
| Virtual desktops | 1–8 desktops, Task View (Super+Tab), Ctrl+Super+←/→, Super+1…9 |
| Desktop | Right-click menu: next wallpaper, display settings, personalise, open Files, open Terminal |
| Shortcuts | Super (start), Super+A/N/S/E/I/L/D, Print and Super+Shift+S (screenshot), Ctrl+Alt+T and Super+Enter (terminal), Ctrl+Shift+Esc (System Monitor), Alt+F4 |
| Applications | File Manager (copy/move/trash/restore, open-with, properties, hidden files, grid/list); Settings (system info, display, sound, network, appearance, taskbar, multitasking, keyboard and mouse, notifications, date and time, user account); Terminal (foot, Meridian theme); System Monitor (CPU/memory/disk/network graphs, process list with end task); Text Editor (find/replace, go to line, line numbers, encodings, unsaved-changes guard) |
| Session | Graphical sign-in (PAM + logind) with live autologin; screen lock (swaylock); sign out, restart, shut down |
| Boot | GRUB menu on BIOS and UEFI: live, safe graphics (software rendering), text console, reboot, power off; psplash boot splash |
| Rendering | GPU (VMware SVGA 3D, virtio-gpu/virgl, Intel) or automatic software fallback (pixman + llvmpipe) for VirtualBox/QEMU 2D adapters |
| Installer | See below |

Features that aren't implemented are not offered as controls. Examples:
- no brightness slider (VMs have no backlight);
- no Bluetooth;
- no printer settings.

### Installer (optional)

**Install CustomOS** appears in the start menu only in the live session.
It asks for:
1. a target disk;
2. your name, user name, computer name and password;
3. an explicit confirmation.

Safeguards:
- The disk list shows model and size. Disks smaller than 8 GB are disabled.
- The confirm page names the disk being erased. It requires ticking "I understand everything on … will be erased".
- The privileged script refuses unless `--confirm` repeats the exact disk path.
- It also refuses if any partition of the disk is mounted.
- Nothing is erased silently.

The installed layout of the base edition is GPT: a 1 MiB BIOS-boot
partition, a 512 MiB EFI system partition (mounted at `/boot`) and an ext4
root. The OTA edition's installer creates the A/B layout instead (two
read-only system slots and a data partition; see
[docs/OTA-UPDATES.md](docs/OTA-UPDATES.md#ab-disk-layout-installed-systems)). GRUB is installed for
both UEFI and legacy BIOS, so the disk boots either way. On the installed
system:
- the live user, autologin and passwordless sudo are removed;
- the new account is an administrator through `sudo` (wheel group);
- root is locked.

## What was verified

These ran in the development environment: Qt 6.4, labwc 0.7 headless with
the pixman renderer, and screenshots taken with grim.
- The desktop, all five applications, the greeter and the installer UI compile and start.
- The shell renders correctly. The start menu, quick settings, calendar, notification centre, toasts, task view and context menus open.
- Live theme, accent and wallpaper changes propagate.
- The sign-in daemon's real PAM path rejects a wrong password, and starts the session as the user with privileges dropped.
- Both defconfigs resolve with real Buildroot 2026.02.3 with no Kconfig
  warnings and every requested option kept (`tests/check-buildroot-config.sh`).
  `make show-info` and `check-package` pass. This found and fixed a real
  error: a recursive Kconfig dependency in `meridian-desktop`. The resolved
  base configuration is identical before and after the fix.
- Every symbol in `linux.fragment` exists in Linux 6.19's Kconfig.

Over-the-air updates:
- **Unit tests** of the update service (`updater/tests`, 5 suites):
  - version order;
  - manifest and `SHA256SUMS` parsing;
  - state and history;
  - the GitHub client against a local HTTP server: stable and beta
    channels, a repository without releases, rate limits, no plain HTTP,
    the token not sent to the storage host, resumed and oversized
    downloads;
  - the whole check → download → verify → install → confirm flow against
    a fake RAUC on D-Bus, including corrupted and untrusted downloads, a
    signed version that differs from the release, a failed installation,
    a rolled-back boot and the live session.
- **Release tooling** with real RAUC: `release/make-release.sh` builds and
  signs a verity bundle, verifies it like a target does, and refuses a
  bundle signed by another CA.
- **End-to-end in QEMU** (`tests/e2e`; on real partitions with real GRUB,
  RAUC and systemd), using a *test fixture* root file system built from
  Ubuntu packages, not CustomOS:
  1. the A/B installer;
  2. first boot (BIOS and UEFI);
  3. an update from a mock GitHub API, with files, accounts and settings
     kept;
  4. manual rollback;
  5. automatic rollback of a version that fails its health check, and of
     one that panics on every boot;
  6. refusal of a bundle signed with an untrusted key, and of a downgrade.

  See [tests/e2e/README.md](tests/e2e/README.md).
- **Settings › System › Updates and Meridian Update**, rendered against a
  mock of the update service in every state: up to date, available,
  downloading, restart required, rolled back, signature error, live
  session; light and dark.

These have **not** run:
- the Buildroot build itself, so no ISO and no OTA image or bundle built by
  Buildroot;
- the kernel;
- booting in VirtualBox or VMware;
- GPU paths;
- the base edition's installer against a real disk (the A/B installer ran
  in QEMU, see above);
- RAUC 1.15 as Buildroot builds it (the tests used Ubuntu's RAUC 1.11);
- the GitHub Actions workflows, and publishing to real GitHub Releases (the
  tests used a mock of the API);
- the newest-feature labwc 0.9.8 keybinds (`onRelease`, `UnSnap`, etc. — the test host had 0.7).

Expect to fix small issues on the first real build. The places most likely
to need attention:

1. **Recipes for the six new packages.** labwc, libsfdo, layer-shell-qt,
   extra-cmake-modules, swaylock and grim have no `.hash` files yet, because
   their tarballs could not be downloaded here to compute checksums.
   Buildroot warns and continues. After a first successful download, add
   hashes with `utils/add-custom-hashes`, or by writing `sha256  <hash>
   <file>` lines next to each `.mk`.
2. **The UEFI GRUB image** must fit the 1 MiB FAT image Buildroot creates
   inside the ISO. The module list is kept short for that reason.
3. **Live-image size and RAM.** The root file system is loaded into RAM as
   a zstd initramfs. Give the VM at least 3 GB of RAM; 4 GB is recommended.

## Licences

The desktop code and generated artwork are MIT licensed (`desktop/LICENSE`).
Inter and JetBrains Mono are under the SIL Open Font License. The
`tux-sunset` wallpaper was supplied by the project owner. Every built
package's licence is collected by `make legal-info` in
`output/legal-info/`.

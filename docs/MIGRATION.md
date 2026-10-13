# Moving to the update-capable edition

Over-the-air updates need a different disk layout (two system slots and a
data partition) and an image built with the updater. Neither can be added to a
CustomOS 0.1 system that is already installed or running. This page covers:
1. your existing Buildroot build;
2. the first build of the OTA edition;
3. computers that already run CustomOS 0.1.

## 1. Your existing build in `output/` is not affected

The update system is a separate Buildroot configuration,
`customos_x86_64_ota_defconfig`, built by `./build.sh --ota` into its **own
directory, `output-ota/`**. `output/` is never configured, cleaned or written
by it.

What changed in files the existing build reads:

| File | Change | Effect on a build in progress |
|---|---|---|
| `configs/customos_x86_64_defconfig` | `BR2_GLOBAL_PATCH_DIR` added: rsync 3.4.3 is patched to build with the toolchain's Linux 5.4 headers (see below) | none until the configuration is applied again |
| `board/customos/linux.fragment`, `post-image.sh` | none | – |
| `package/meridian-desktop/Config.in` | Kconfig loop fixed (`select` instead of `depends on` linux-pam) | none: the resolved `.config` is **identical** before and after the fix (compared with `diff`). The old file printed `recursive dependency detected!`, which Kconfig reports but tolerates |
| `br2-external/Config.in` | adds the `customos-updater` package | none: it is off unless selected |
| `board/customos/post-build.sh` | takes the version from `VERSION`; adds `BUILD_ID` and `/etc/issue` | only the text of `/etc/os-release` and `/etc/issue`, and only if the build runs it after you pull |
| `desktop/` | Settings › System › Updates, the Meridian Update app, new icons | none until `meridian-desktop` is rebuilt (below). The *Updates* card, the Meridian Update app and its notifications appear only where the update service is installed, so the base edition shows none of them |

So you can let the current build finish untouched. Two caveats, both
harmless:
- The build that is running now uses the *old* `post-build.sh`, so its
  `/etc/os-release` says `VERSION_ID=1.0`. In the numbering of
  [VERSIONING.md](VERSIONING.md) that image is **0.1.0**. It has no update
  service, so the number never meets the update logic.
- If you pull these changes and `make` runs again in `output/`, nothing is
  rebuilt. Buildroot doesn't notice changed sources of `desktop/` on its
  own; `make … meridian-desktop-rebuild all` picks them up. Only
  `post-build.sh` runs again, and it writes the version from `VERSION`
  (`0.2.0`) into `/etc/os-release`.

### If your build stopped at rsync

`syscall.c:38:10: fatal error: linux/openat2.h: No such file or directory`
comes from rsync 3.4.3, which needs kernel headers from Linux 5.6 or newer. The
Bootlin toolchain in the configuration has Linux 5.4 headers.
`br2-external/patches/rsync/3.4.3/` holds the upstream fix (from rsync 3.4.4).
To use it in an existing `output/`:

```sh
make -C buildroot O=$PWD/output BR2_EXTERNAL=$PWD/br2-external customos_x86_64_defconfig
make -C buildroot O=$PWD/output rsync-dirclean all
```

The first line applies the configuration again. Packages already built are
kept. `rsync-dirclean` removes only rsync's half-built directory, so it is
extracted and patched again.

## 2. Building the OTA edition for the first time

Once:
1. Create the signing keys and commit the public certificate
   ([SIGNING.md](SIGNING.md)):
   ```sh
   release/keys/create-ota-keys.sh
   git add keys/customos-ota-ca.cert.pem && git commit -m "Add the OTA root certificate"
   ```
2. Build:
   ```sh
   ./build.sh --ota          # or ./build.sh --ota --docker
   ```

**This is a full build**, not an incremental one. It takes about as long as
your first build: 1–3 hours on 8 cores.
- Downloads in `buildroot/dl/` are shared, so only the new sources are
  fetched: RAUC, OpenSSL, squashfs-tools, json-glib, lz4 and lzo.
- ccache (`~/.buildroot-ccache`) speeds up compiling.
- `output-ota/` needs another ~30 GB of disk.

### Why not reuse `output/`?

Buildroot doesn't track configuration changes for packages that are already
built. Switching `output/` to the OTA configuration would leave stale
results:
- **OpenSSL is new, and nine packages already built would use it:**
  - `qt6base`, which needs it for the HTTPS downloads; without a rebuild
    the update service can't reach GitHub;
  - `systemd`, `sudo`, `linux-pam`, `coreutils`, `kmod`, `pipewire`,
    `rsync` and `wpa_supplicant`.
- **The kernel** gets new options (`linux-ota.fragment`: dm-verity, loop,
  squashfs, overlayfs, watchdogs) and must be configured again.
- **New packages:** RAUC, OpenSSL, squashfs-tools, json-glib and the
  updater, plus host RAUC for signing.
- **squashfs** is added as a second root file system image.

(Measured: the base configuration has 225 packages and the OTA one 237.
The list above is the difference between `make show-info` of the two.)

Buildroot's manual (*Understanding when a full rebuild is necessary*) asks
for a full rebuild after changes like these. A full rebuild in `output/`
would take as long as building `output-ota/`, and it would replace your 0.1
build. Reusing `output/` therefore gains nothing. The separate `output-ota/`
is the reliable path, and the one the release workflow uses.

## 3. Computers that already run CustomOS 0.1

| You have | What to do |
|---|---|
| The 0.1 **live ISO** (nothing installed) | Use the 0.2.0 ISO instead. The live session has no updates: it runs from read-only media. Settings explains this, and points to *Install CustomOS* |
| 0.1 **installed** to a disk | Back up, then reinstall from the 0.2.0 ISO (below) |

An installed 0.1 system can't be converted in place:
- It has one ext4 root file system that fills the disk.
- Making room for the A/B system slots would mean shrinking the running root
  file system. A power cut or error during that could lose everything.

The 0.2.0 installer creates the new layout from scratch.

To move an installed 0.1 system to 0.2.0:
1. Copy your files from `/home/<you>` to a USB disk or another computer.
   Desktop settings live in hidden folders there (`~/.config`, `~/.local`),
   so copy those too.
2. Boot the 0.2.0 ISO and choose **Install CustomOS**. It erases the disk.
3. Sign in and copy your files back.

From then on, updates arrive through Settings › System › Updates, and the
files on the data partition are kept across updates
([OTA-UPDATES.md](OTA-UPDATES.md#what-survives-an-update)).

## Summary

| Step | Rebuild needed? |
|---|---|
| Finishing your current 0.1 build in `output/` | No; its configuration is unchanged |
| First OTA build | Yes, full build in `output-ota/` (downloads and ccache are shared) |
| Later releases | Incremental `./build.sh --ota` in `output-ota/` |
| 0.1 installed → 0.2.0 | Reinstall, after backing up `/home` |
| 0.2.0 → later versions | No: over-the-air updates |

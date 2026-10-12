# End-to-end test of the A/B update system

`run-e2e.py` installs a system with the A/B installer, then updates it, rolls
it back and attacks it, in QEMU. Real partitions, GRUB (BIOS and UEFI),
RAUC, the update service and a mock of the GitHub API are used throughout.

> **This tests a fixture, not CustomOS.** Building a real CustomOS image
> needs the full Buildroot build. The fixture therefore takes a small root
> file system made from Ubuntu 24.04 packages (systemd, RAUC, GRUB, Qt 6
> libraries, an Ubuntu kernel). It installs into it the **CustomOS update
> components exactly as built from `updater/`**:
> - `customos-updated` and `customos-update`;
> - `customos-preinit` and `ota-lib.sh`;
> - the A/B GRUB menu;
> - the health check;
> - the RAUC configuration and hooks;
> - `customos-install-ab`.
>
> The desktop is replaced by a stand-in service. The fixture is never shipped.
> What it can't show: CustomOS's own kernel configuration, the Buildroot
> image, and the graphical parts.

## What it checks

| Stage | Scenario |
|---|---|
| `install` | In a live environment, `customos-install-ab` partitions an empty 10 GB disk, writes and verifies slot A, creates the account and installs GRUB |
| `boot` | First boot (BIOS): slot A read-only, `/etc` an overlay on the data partition, the account from the installer, live user and autologin removed, health check confirms the slot |
| `update` | 0.3.0 found on the mock GitHub, downloaded, verified, installed into slot B; restart; trial boot confirmed; files, account, computer name and network connections kept, unsupported `/etc` edits moved aside |
| `rollback` | *Go back* to 0.2.0 (slot A); history; then 0.3.0 again |
| `broken` | 0.4.0's desktop stand-in fails: the health check marks it bad, the machine returns to 0.3.0 by itself, history says *rolled back* |
| `panic` | 0.5.0 panics on every boot: after three tries GRUB returns to 0.3.0 |
| `untrusted` | 0.6.0 is signed with a key the system doesn't trust: refused, nothing written |
| `downgrade` | A genuinely signed but older bundle given straight to `rauc install` is refused by the pre-install hook |
| `uefi` | The same disk boots with OVMF (UEFI) |

## Running it

On an Ubuntu 24.04 host, as root. KVM is used if `/dev/kvm` exists;
without it, the whole run takes about 25 minutes.

```sh
apt install qemu-system-x86 ovmf mmdebstrap rauc grub-pc-bin grub-efi-amd64-bin \
    squashfs-tools dosfstools gdisk e2fsprogs python3-pexpect \
    qt6-base-dev cmake g++ openssl
W=/var/tmp/customos-e2e; mkdir -p $W

# 1. Base root file system (the fixture)
mmdebstrap --variant=minbase --mode=root --components="main universe" \
  --include=systemd,systemd-sysv,udev,dbus,rauc,rauc-service,grub-common,grub2-common,grub-pc-bin,grub-efi-amd64-bin,e2fsprogs,dosfstools,gdisk,fdisk,util-linux,mount,kmod,passwd,login,procps,iproute2,systemd-resolved,libqt6core6t64,libqt6network6t64,libqt6dbus6t64,ca-certificates,less,squashfs-tools,cpio \
  noble $W/base \
  "deb http://archive.ubuntu.com/ubuntu noble main universe" \
  "deb http://archive.ubuntu.com/ubuntu noble-updates main universe" \
  "deb http://security.ubuntu.com/ubuntu noble-security main universe"

# 2. A kernel, unpacked (it goes into the system slots, not the host)
mkdir -p $W/kpkg && cd $W/kpkg
K=$(apt-cache depends linux-image-virtual | grep -oE 'linux-image-[0-9][^ ]*' | head -1)
apt-get download $K ${K/linux-image/linux-modules}
for d in *.deb; do dpkg-deb -x $d x; done
cd -

# 3. The update components, and test keys (two sets: trusted and untrusted)
cmake -S updater -B $W/updater -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
cmake --build $W/updater -j
release/keys/create-ota-keys.sh --dir $W/keys --no-passphrase --no-install
release/keys/create-ota-keys.sh --dir $W/evil --no-passphrase --no-install

# 4. Images and releases (0.2.0 … 0.6.0), then the test
tests/e2e/build-images.sh --base $W/base --kernel $W/kpkg/x --updater-build $W/updater \
    --keys $W/keys --evil-keys $W/evil --out $W/out
tests/e2e/run-e2e.py --images $W/out              # or --stages install,boot,update
```

Each machine's serial console is logged in `$W/out/logs/<stage>.log`. The
stages share one virtual disk, so they run in order from `install`.

The releases are produced by the real `release/make-release.sh`, signed with
the test keys, and served by `mock-github.py`. That server implements the
parts of the GitHub API the update service uses: `releases/latest`, the
release list, redirects to a storage host, HTTP Range requests and asset
digests.

# Architecture

## Boot

The ISO is built by xorriso through Buildroot. It contains:
- GRUB (El Torito BIOS image, plus an EFI FAT image);
- `bzImage`;
- the root file system as a zstd-compressed initramfs.

Boot sequence:
1. GRUB shows the menu from `board/customos/grub.cfg`.
2. The kernel unpacks the root into RAM.
3. Buildroot's `/init` mounts devtmpfs and executes systemd.
4. psplash shows the splash screen until `meridian-login.service` starts on tty1.

## Session

`meridian-login` is a small root daemon. Its loop:
1. On the first pass it reads `/etc/meridian/login.conf`. If the file names an `autologin` user, it starts that user's session through the `meridian-autologin` PAM service.
2. Otherwise it runs `meridian-greeter`. The greeter is a Qt application on linuxfb, unprivileged, talking to the daemon over pipes.
3. The daemon authenticates against PAM. pam_systemd then opens a logind session on tty1 and seat0.
4. It drops privileges and runs `meridian-session` as the user's login shell.

`meridian-session` then:
1. picks hardware or software rendering (pixman + llvmpipe) from the DRM driver and `meridian.software=1`;
2. inside VMs, exports a default resolution;
3. execs **labwc**. If labwc dies within 15 s on the GPU path, it retries in software mode.

labwc's `autostart` starts **meridian-shell** under a restart loop.

## Desktop shell (`desktop/shell`)

A Qt Quick client of `zwlr_layer_shell_v1`, using LayerShellQt. It has these
surfaces:

| Surface | Layer | Purpose |
|---|---|---|
| Background | background | wallpaper, desktop menu |
| Taskbar | top, exclusive zone 48 px | start, task view, pinned/running apps, tray, clock |
| Overlay | overlay, keyboard focus | start menu, quick settings, calendar, notification centre, task view, context menus |
| Toasts, OSD, tooltip | overlay, input-region limited | notifications, volume OSD, hover tooltips |

Protocols:
- Windows come from `zwlr_foreign_toplevel_management_v1`: list, activate, minimise, maximise, close.
- Virtual desktops come from `ext_workspace_v1`.

The shell owns the session's `org.freedesktop.Notifications` name. It writes
labwc's `rc.xml` and `environment` from the user's settings, then reloads
labwc with SIGHUP. Keyboard layout, repeat, pointer and touchpad settings,
the window theme, corner radius and desktop count all work this way. Global
shortcuts are labwc keybinds that call `meridian-shell --action NAME` over a
local socket.

## Shared core (`desktop/core`)

The `Meridian.Core` QML module provides:
- **Settings**: `~/.config/meridian/desktop.conf`, watched for live changes, shared by all processes.
- **Theme** tokens (light/dark, accent).
- **Icon provider**: generated symbolic SVG icons, recoloured at load.
- **Services:** Apps (desktop entries, MIME defaults), Sys, SystemInfo, Audio (`wpctl`/`pw-dump`), Network (`nmcli`) and Display (`wlr-randr`).
- Shared controls.

`MeridianStyle` is a complete Qt Quick Controls style.

## Applications

Every application is a Qt Quick program linked statically against the core
and style modules:

| Application | Binary | Built on |
|---|---|---|
| File Manager | `meridian-files` | DirModel, FileOps (XDG trash) |
| Settings | `meridian-settings` | `timedatectl`, `hostnamectl`, `chpasswd` via sudo |
| System Monitor | `meridian-monitor` | `/proc` |
| Text Editor | `meridian-editor` | QSaveFile, encoding detection |
| Terminal | `foot` | Meridian config |
| Installer | `meridian-installer` | `customos-install` run through `sudo -n` |

## Installer

`/usr/libexec/customos-install` installs the system. It:
1. validates every argument and requires `--confirm` to equal `--disk`;
2. partitions with sgdisk (BIOS-boot + ESP + ext4);
3. copies the running root with rsync, excluding the live-only files;
4. writes fstab, the hostname and the user account;
5. installs GRUB for x86_64-efi (removable path) and i386-pc;
6. writes `grub.cfg` with live, safe and text entries.

Progress is reported to the GUI as `PROGRESS <n> <text>` lines.

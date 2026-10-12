# Running CustomOS in a virtual machine

You need `CustomOS-x86_64.iso`, produced by `./build.sh` (see the README).
The ISO boots with both legacy BIOS and UEFI firmware.

Recommended VM resources:
- **4 GB RAM** (at least 3 GB). The live system runs entirely from RAM.
- **2 or more CPUs.**
- **25 GB disk** if you want to install. The installer needs at least 8 GB.

## VirtualBox 7.x

1. **New** › Name `CustomOS`, ISO image: `CustomOS-x86_64.iso`.
   - Type **Linux**, Version **Other Linux (64-bit)**.
   - Tick **Skip Unattended Installation**.
2. Hardware: 4096 MB memory, 2 CPUs.
   - *Enable EFI* is optional; both firmware types work.
3. Hard disk: create a 25 GB VDI (dynamically allocated).
4. Before the first start, open **Settings**:
   - **Display › Screen**:
     - Graphics Controller **VMSVGA**.
     - Video Memory **128 MB**.
     - **Enable 3D Acceleration** optional. With it, the desktop uses the GPU through Mesa's SVGA driver. Without it, CustomOS detects the 2D-only adapter and renders in software, which is slower but reliable.
   - **System › Pointing Device**: **USB Tablet**, for seamless mouse integration.
   - **Audio**: Intel HD Audio (the default) works.
   - **Network**: NAT works (the Intel PRO/1000 and virtio adapters are supported).
5. Start the VM. At the boot menu choose **Start CustomOS (live)**.
   - The desktop signs in as the user `live` (password `live`).

Display size: without guest tools the virtual screen does not follow the
window size. Inside a VM, CustomOS switches to 1600×900 on start. Choose
another resolution in **Settings › Display**.
- For a different default, add `meridian.mode=1920x1080` to the kernel line.
  To do that, press `e` on the boot menu entry.

## VMware Workstation / Player / Fusion

1. **Create a New Virtual Machine** › Installer disc image: `CustomOS-x86_64.iso`.
   - Guest OS: **Linux**, version **Other Linux 6.x kernel 64-bit**.
2. 25 GB disk, 4 GB memory, 2 processors.
3. **Customize Hardware › Display**: tick **Accelerate 3D graphics** (recommended).
4. Firmware (optional): VM Settings › Options › Advanced › **UEFI**, or keep BIOS.
5. Power on and choose **Start CustomOS (live)**.

VMware's SVGA adapter, VMXNET3 or e1000 networking, the LSI Logic/PVSCSI/SATA/NVMe
disk controllers, HD Audio and ES1371 sound are all built into the kernel.

## QEMU / virt-manager

```sh
qemu-img create -f qcow2 customos.qcow2 25G
qemu-system-x86_64 -enable-kvm -m 4096 -smp 2 -cpu host \
  -device virtio-vga-gl -display gtk,gl=on \
  -device virtio-net,netdev=n0 -netdev user,id=n0 \
  -device intel-hda -device hda-duplex \
  -drive file=customos.qcow2,if=virtio -cdrom CustomOS-x86_64.iso -boot d
# UEFI: add  -bios /usr/share/ovmf/OVMF.fd
```

Without `gl=on`, use `-vga std` and the desktop renders in software.

## Boot menu entries

| Entry | Use |
|---|---|
| Start CustomOS (live) | Normal start |
| Start CustomOS (safe graphics) | `nomodeset meridian.software=1`: firmware framebuffer and CPU rendering. Use it if the screen stays black or the desktop fails to appear |
| Start CustomOS (text console only) | No desktop; sign in on the console as `live` / `live` |
| Reboot, Power off | |

If the desktop cannot start on the GPU, the session retries in software
mode by itself. Diagnostics are in `~/.local/state/meridian/session.log`
and `journalctl -b`.

## Installing to the virtual disk

1. In the live session open **Start › Install CustomOS**.
2. Pick the 25 GB disk, enter your account details, tick the erase
   confirmation and press **Install**.
3. When it reports *Installation complete*, press **Restart**. Remove the ISO
   (VirtualBox: Devices › Optical Drives › Remove disk) so the VM boots from
   the disk.
4. Sign in with the account you created. It can use `sudo`.

## Keyboard shortcuts

| Keys | Action |
|---|---|
| Super | Start menu |
| Super+A / Super+N | Quick settings / notifications |
| Super+Tab | Task view (virtual desktops) |
| Alt+Tab, Alt+Shift+Tab | Switch windows |
| Super+← / → / ↑ / ↓ | Snap left / right, maximise, restore |
| Super+H | Minimise |
| Super+D | Show desktop |
| Ctrl+Super+← / → | Previous / next desktop |
| Shift+Super+← / → | Move window to previous / next desktop |
| Super+1…9 | Taskbar app 1…9 |
| Super+E / Super+I | Files / Settings |
| Super+Enter, Ctrl+Alt+T | Terminal |
| Ctrl+Shift+Esc | System Monitor |
| Super+L | Lock |
| Print, Super+Shift+S | Screenshot to ~/Pictures/Screenshots |
| Alt+F4 | Close window |
| Alt+Space | Window menu |

Inside VirtualBox, the Super key may be captured by the host. Use
**Input › Keyboard › Insert Key Combination**, or press the key while the VM
window has keyboard capture.

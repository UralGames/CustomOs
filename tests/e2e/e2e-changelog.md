# Changelog of the end-to-end test versions

## 0.2.0

Test fixture: the version the installer puts on the disk.

## 0.3.0

Test fixture: a good update.

- Starts like 0.2.0, with a new version number.

## 0.4.0

Test fixture: its desktop stand-in fails to start, so the health check must
reject it and the computer must return to the previous version.

## 0.5.0

Test fixture: its first process exits, so the kernel panics on every boot and
GRUB's boot counter must fall back to the previous slot.

## 0.6.0

Test fixture: signed with a key the system does not trust, so it must never be
installed.

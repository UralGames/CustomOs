# Patches for Buildroot's own packages

Both defconfigs set `BR2_GLOBAL_PATCH_DIR` to this directory. Buildroot
applies `<package>/<version>/*.patch` after its own patches for that package.
Patches go in a directory named after the version they were made for. When
Buildroot moves to another version, the patch is no longer applied, and
can't break the build.

| Patch | Why | Remove when |
|---|---|---|
| `rsync/3.4.3/0001-build-openat2-autodetect.patch` | rsync 3.4.3 includes `<linux/openat2.h>` (Linux 5.6+) on every Linux build. The Bootlin x86-64-v2 glibc stable toolchain has Linux 5.4 headers, so `syscall.c` doesn't compile. Upstream fix from rsync 3.4.4 | Buildroot ships rsync 3.4.4 or later, or the toolchain has Linux 5.6+ headers |

The *Build ISO* workflow builds every package that has a directory here before
the full build starts (job *Build the patched packages*).

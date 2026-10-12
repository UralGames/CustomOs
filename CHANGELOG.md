# CustomOS changelog

Versions follow [Semantic Versioning](https://semver.org): MAJOR.MINOR.PATCH.
The release script copies the section of a version into the update manifest,
so these notes are what Settings › System › Updates shows before installing.
Add a section for every release before tagging it (see docs/RELEASING.md).

## 0.2.0 — unreleased

The first version that can update itself over the air.

- Over-the-air updates from GitHub Releases, verified with a trusted signature
  (RAUC bundles) before anything is installed.
- New A/B disk layout: updates install into the inactive system slot while
  CustomOS keeps running; a restart switches over, and a version that fails to
  start is rolled back automatically.
- Settings › System › Updates and the Meridian Update app: check for updates,
  release notes, download and installation progress, restart, history and
  rollback to the previous version.
- Your files, accounts and supported settings live on a separate data
  partition and are kept across updates.
- Installing CustomOS 0.1 to a disk used a single-partition layout that cannot
  be updated safely. Back up your files and reinstall from the 0.2.0 ISO to get
  updates (see docs/MIGRATION.md).

## 0.1.0 — initial release

- Live ISO with the Meridian desktop (BIOS and UEFI).
- Optional installation to a disk (single root partition).

# Versions

CustomOS uses [Semantic Versioning 2.0.0](https://semver.org): `MAJOR.MINOR.PATCH`,
optionally with a pre-release part (`0.3.0-rc.1`).

| Version | Meaning |
|---|---|
| 0.1.0 | Initial release: live ISO and single-partition installer (no updates) |
| 0.2.0 | First version with over-the-air updates and the A/B layout |
| 0.2.x | Fixes only |
| 0.x.0 | New features; may need a newer minimum version (`--min-version`) |
| 1.0.0 | First version declared stable |

## Where the version is written

- **`VERSION`** (repository root) is the single source. It holds one line,
  e.g. `0.2.0`.
- `board/customos/post-build.sh` writes it into the image:
  - `/usr/lib/os-release`: `VERSION_ID=0.2.0`, `VERSION="0.2.0 (Meridian)"`,
    and `BUILD_ID` = the Git commit or `$CUSTOMOS_BUILD_ID`;
  - `/etc/issue`.
- The release script takes it from the built image, checks it against
  `VERSION` and the Git tag, and writes it into the bundle's signed manifest
  and the release manifest.
- The Git tag and GitHub release are named `vX.Y.Z`.
- `CHANGELOG.md` has one `## X.Y.Z` section per release. That text becomes
  the release notes shown in Settings › System › Updates.

The update service compares versions by Semantic Versioning precedence:
`0.2.0 < 0.2.1 < 0.3.0-rc.1 < 0.3.0 < 0.10.0`. It installs only versions that
are strictly newer than the running one.

## Channels

- **stable** (the default): the newest GitHub release that is not marked as
  a pre-release. Versions with a pre-release part are always published as
  GitHub pre-releases, so the stable channel never sees them.
- **beta** (reserved): the newest release including pre-releases. The update
  service already understands `Channel=beta`. The release workflow and the
  Settings page don't offer it yet. To enable it later:
  1. Publish `X.Y.Z-beta.N` versions with `release/make-release.sh --channel beta`.
  2. Let people choose the channel in Settings (it writes
     `/etc/customos/update.conf.d/channel.conf`).

## Compatible string

Bundles carry a *compatible* string, `customos-x86_64-ab`, and a system
installs only bundles with its own. Change it only for a new, incompatible
family of systems (for example another architecture or partition layout); it
is configured in `BR2_PACKAGE_CUSTOMOS_UPDATER_COMPATIBLE`.

## Minimum version

`release/make-release.sh --min-version 0.3.0` writes `"min_version"` into the
manifest. Older systems then report that they need an intermediate update
first, instead of installing a version that can't handle their data. Use it
when a release migrates data in a way older versions can't skip.

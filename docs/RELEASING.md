# Publishing a release

Prerequisites, done once: the signing setup in [SIGNING.md](SIGNING.md).

## 1. Prepare

1. Set the version in `VERSION` (see [VERSIONING.md](VERSIONING.md)).
2. Add a `## X.Y.Z` section to `CHANGELOG.md`. People read it in Settings
   before they install, so write it for them.
3. Commit and push.

## 2a. Build and publish with GitHub Actions

Push a tag:

```sh
git tag v0.3.0
git push origin v0.3.0
```

`.github/workflows/release.yml` then:
1. checks that the tag equals `VERSION`, that `CHANGELOG.md` has the
   section, that `keys/customos-ota-ca.cert.pem` exists and that the signing
   certificate in the secrets is issued by that CA;
2. builds the OTA edition (`./build.sh --ota`), with download and compiler
   caches;
3. creates the signed bundle, manifest, `SHA256SUMS` and the ISO
   (`release/make-release.sh`);
4. keeps them as a workflow artifact for 14 days;
5. publishes the GitHub release (`release/publish-github-release.sh`): it
   is created as a draft, the files are uploaded, and only then is it
   published, so update clients never see a half-uploaded release.

The workflow can also be started by hand (*Actions › Release › Run
workflow*). It then publishes only if *publish* is ticked.

**Build time.** A full build with Qt, Mesa and LLVM takes several hours.
GitHub-hosted runners stop jobs after 6 hours. The caches make later builds
much faster, but the first one may not fit. It almost certainly won't in a
**private** repository: its standard runners have 2 CPU cores, against 4
for public repositories. Options:
- a self-hosted runner: set the repository variable `CUSTOMOS_BUILD_RUNNER`
  to its label, and `CUSTOMOS_BUILD_TIMEOUT` (minutes) if needed;
- a larger GitHub runner;
- building locally (2b).

## 2b. Build and publish from your own machine

```sh
./build.sh --ota                                   # output in output-ota/
release/make-release.sh output-ota/images \
    --cert ~/.customos-signing/signing.cert.pem --key ~/.customos-signing/signing.key.pem
release/publish-github-release.sh release/out/v0.3.0          # uses gh, or $GITHUB_TOKEN
```

`publish-github-release.sh --draft` leaves the release as a draft for review.

**Private repository.** Installed systems check releases without signing
in. GitHub answers *not found* for a private repository, so they can't see
its releases. Before the first release, do one of these
([OTA-UPDATES.md](OTA-UPDATES.md#configuration)):
- make the repository public;
- publish releases to a separate public repository (it needs at least one
  commit, e.g. a README):
  1. set `BR2_PACKAGE_CUSTOMOS_UPDATER_REPOSITORY` to it;
  2. in this repository's Actions settings, set the variable
     `CUSTOMOS_RELEASE_REPO` to `OWNER/NAME`;
  3. set the secret `CUSTOMOS_RELEASE_TOKEN` to a fine-grained token with
     *Contents: read and write* on that repository.

  Locally, use `publish-github-release.sh --repo OWNER/NAME --target ""`;
- give each computer a read-only token.

## 3. After publishing

- Installed systems see the update at their next check (within six hours,
  or immediately with *Check for updates*).
- To withdraw a release, delete or un-publish it on GitHub before systems
  install it. Already installed copies are not removed; publish a fixed,
  higher version instead. The update service never installs an older
  version, so you can't "roll forward" to a lower number.
- Users can always go back to the version they had before through
  Settings › System › Updates › *Go back*.

## Local test without GitHub

`tests/e2e/mock-github.py --root DIR` serves release directories (as written
by `make-release.sh`) with the same API as GitHub. Point a system at it with:

```ini
# /etc/customos/update.conf.d/50-test.conf
[Source]
ApiUrl=http://HOST:8077
Repository=test/customos
[Updates]
AllowInsecureHttp=true
```

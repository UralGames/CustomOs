# Signing keys and GitHub secrets

CustomOS systems install only update bundles signed by a key they trust. This
page explains which keys exist, where each one lives, and how to set up
GitHub Actions or a local machine to sign releases.

## The keys

| File | What it is | Where it lives |
|---|---|---|
| `ca.key.pem` | Root CA private key | **Offline**, e.g. on two USB sticks in different places. Never on GitHub, in CI or in an image |
| `ca.cert.pem` | Root CA certificate (public) | Committed as `keys/customos-ota-ca.cert.pem`; every image installs it as `/etc/rauc/keyring.pem` |
| `signing.key.pem` | Release signing key | GitHub secret `CUSTOMOS_SIGNING_KEY`, or your signing machine |
| `signing.cert.pem` | Release signing certificate (issued by the CA, *code signing* usage, valid 5 years) | GitHub secret `CUSTOMOS_SIGNING_CERT` |

The images trust the CA, not a particular signing key. A leaked signing key
can therefore be replaced without changing the images. Only a leaked CA key
forces new images.

## One-time setup

1. Create the keys on a machine you trust (not a shared CI runner):

   ```sh
   release/keys/create-ota-keys.sh            # asks for a passphrase for the CA key
   ```

   The keys go to `~/.customos-signing/`. The script refuses to write them
   inside the repository. It copies the public CA certificate to
   `keys/customos-ota-ca.cert.pem`.

2. Commit the public certificate:

   ```sh
   git add keys/customos-ota-ca.cert.pem
   git commit -m "Add the CustomOS OTA root certificate"
   ```

   Every image built from now on trusts this CA. The OTA build stops with an
   explanation if the file is missing.

3. Store the signing key and certificate as GitHub secrets (repository
   *Settings › Secrets and variables › Actions › New repository secret*),
   pasting each whole PEM file:

   | Secret | Value |
   |---|---|
   | `CUSTOMOS_SIGNING_KEY` | contents of `~/.customos-signing/signing.key.pem` |
   | `CUSTOMOS_SIGNING_CERT` | contents of `~/.customos-signing/signing.cert.pem` |

   With the GitHub CLI:

   ```sh
   gh secret set CUSTOMOS_SIGNING_KEY  < ~/.customos-signing/signing.key.pem
   gh secret set CUSTOMOS_SIGNING_CERT < ~/.customos-signing/signing.cert.pem
   ```

   Use an *environment* (*Settings › Environments*) with required reviewers
   if releases should need a second person's approval.

4. Move `ca.key.pem` offline and delete it from the machine.
   `signing.key.pem` can be deleted from the machine too, unless you also sign
   releases locally.

## Signing without GitHub Actions

`release/make-release.sh` reads the key and certificate from files or from
the environment:

```sh
release/make-release.sh output-ota/images \
    --cert ~/.customos-signing/signing.cert.pem --key ~/.customos-signing/signing.key.pem
# or
CUSTOMOS_SIGNING_CERT_FILE=… CUSTOMOS_SIGNING_KEY_FILE=… release/make-release.sh output-ota/images
```

The script signs the bundle and then verifies it exactly as an installed
system does:
- against `keys/customos-ota-ca.cert.pem`;
- with the *code signing* check;
- at the bundle's signing time.

It refuses to finish if the signature doesn't verify. A bundle signed with the
wrong key can never be published by accident.

Hardware tokens (PKCS#11) are supported by RAUC (`--key 'pkcs11:…'`); see
RAUC's documentation.

## Rotating the signing key

Do this every few years, or immediately if the signing key may have leaked.

```sh
release/keys/create-ota-keys.sh --signing-only   # needs the offline CA key in ~/.customos-signing
gh secret set CUSTOMOS_SIGNING_KEY  < ~/.customos-signing/signing.key.pem
gh secret set CUSTOMOS_SIGNING_CERT < ~/.customos-signing/signing.cert.pem
```

Installed systems accept bundles signed with the new key immediately; no
image change is needed.

**If the old signing key leaked,** bundles it signs keep verifying until the
systems stop trusting it. Two ways to achieve that:
- **Revocation list.** Publish a release whose keyring includes a CRL that
  revokes the old certificate, and set `check-crl=true` in
  `updater/boot/rauc-system.conf`.
- **New CA.** Create a new CA and ship a release (signed with the old,
  still-trusted key) whose keyring contains only the new CA. After that
  release, the old key is useless.

Because certificates are checked at the signing time recorded in the bundle,
expiry alone does not stop a leaked key; revocation does.

## If the CA key is lost or leaked

- **Lost:** you can't issue new signing certificates. Installed systems keep
  accepting bundles from the existing signing key until you ship a release
  with a new CA in its keyring, signed by the old signing key.
- **Leaked:** treat every installed system as able to receive malicious
  updates from whoever holds it. Create a new CA, publish a release signed by
  the current signing key whose keyring contains only the new CA, and tell
  users to install it promptly. Systems that never install it stay exposed;
  reinstalling from a new ISO also fixes them.

## Test keys

`release/keys/create-ota-keys.sh --dir /tmp/test-keys --no-passphrase --no-install`
creates throw-away keys without touching `keys/`. The end-to-end tests use
them, together with a second, untrusted set, to prove that foreign
signatures are refused.

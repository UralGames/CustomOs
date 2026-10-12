# Public update certificate

This directory holds **only public material**:

| File | Purpose |
|---|---|
| `customos-ota-ca.cert.pem` | The CustomOS OTA root CA certificate. Every image of the OTA edition installs it as `/etc/rauc/keyring.pem`, and accepts only update bundles signed by a certificate this CA issued |

It doesn't exist yet, and `./build.sh --ota` stops until it does. Create it
once with:

```sh
release/keys/create-ota-keys.sh
git add keys/customos-ota-ca.cert.pem
```

The script creates the private keys in `~/.customos-signing/`, outside the
repository, and copies only the public CA certificate here. See
[docs/SIGNING.md](../docs/SIGNING.md).

**Never put a private key in this directory.**
- `.gitignore` refuses `*.key`, `*.key.pem`, `*-key.pem` and `keys/private/`.
- The image build stops if the certificate file contains a private key.
- The release workflow checks that the signing certificate in the GitHub
  secrets was issued by this CA.

# Verifying Vix release archives

The current release qualification publishes independently extractable host
archives. For the initial qualified host tuple, `x86_64-linux-gnu`, verify the
GitHub build provenance and the exact archive digest before extraction:

```sh
gh attestation verify vix-x86_64-linux-gnu.tar.gz --repo vixcpp/vix
grep -F '  vix-x86_64-linux-gnu.tar.gz' SHA256SUMS
sha256sum --ignore-missing -c SHA256SUMS
mkdir vix-release
tar -xzf vix-x86_64-linux-gnu.tar.gz -C vix-release
export PATH="$PWD/vix-release/bin:$PATH"
vix --version
```

The bootstrap installer is a separate artifact and has its own provenance
attestation. Verify it the same way before extraction or execution:

```sh
gh attestation verify vix-installer-x86_64-linux-gnu.tar.gz --repo vixcpp/vix
grep -F '  vix-installer-x86_64-linux-gnu.tar.gz' SHA256SUMS
sha256sum --ignore-missing -c SHA256SUMS
```

The host archive's release manifest binds its exact SHA-256 digest and
download location and is authenticated with the release authority's Ed25519
signature. Managed installation verifies the signed manifest and downloaded
artifact before activation. GitHub Actions provenance is the bootstrap
artifact's independent authenticity mechanism; the installer is not trusted
merely because it is adjacent to a checksum file.

Only `x86_64-linux-gnu` is currently assembled by the release qualification
workflow. Other host tuples declared by the distribution parser are not
qualified artifacts. Android cross-compilation and WASI SDK builds are not
native CLI host releases. Windows managed installation remains unsupported.

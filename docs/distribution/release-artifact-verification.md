# Verifying Vix release archives

The current release-qualification workflow assembles versioned candidate host
archives. For the currently packaged host tuple, `x86_64-linux-gnu`, substitute
the exact release version and verify the GitHub build provenance and archive
digest before extraction:

```sh
version=2.9.0 # replace with the exact version shown by the release tag
host_archive="vix-x86_64-linux-gnu-${version}.tar.gz"
gh attestation verify "$host_archive" --repo vixcpp/vix
grep -F "  $host_archive" SHA256SUMS
sha256sum --ignore-missing -c SHA256SUMS
mkdir vix-release
tar -xzf "$host_archive" -C vix-release
export PATH="$PWD/vix-release/bin:$PATH"
vix --version
```

The bootstrap installer is a separate artifact and has its own provenance
attestation. Verify it the same way before extraction or execution:

```sh
bootstrap_archive="vix-installer-x86_64-linux-gnu-${version}.tar.gz"
gh attestation verify "$bootstrap_archive" --repo vixcpp/vix
grep -F "  $bootstrap_archive" SHA256SUMS
sha256sum --ignore-missing -c SHA256SUMS
```

The host archive's release manifest binds its exact SHA-256 digest and
download location and is authenticated with the release authority's Ed25519
signature. Managed installation verifies the signed manifest and downloaded
artifact before activation. GitHub Actions provenance is the bootstrap
artifact's independent authenticity mechanism; the installer is not trusted
merely because it is adjacent to a checksum file.

Only `x86_64-linux-gnu` is currently assembled by the release qualification
workflow. Android and WASI target-SDK archives are separate candidates and do
not imply native CLI host releases. Other host tuples declared by the
distribution parser are not qualified artifacts. Windows managed installation
remains unsupported. Official release publication is disabled pending explicit
approval and qualification.

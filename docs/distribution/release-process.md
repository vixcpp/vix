# Current Vix release inputs

## First install and manual archives

The managed first-install entry is the separately distributed vix-installer
bundle. It contains bin/vix-installer plus the provisioned PUBLIC release
trust store and stable source configuration under share/vix/. It installs
versions below VIX_HOME when explicitly set, otherwise XDG_DATA_HOME/vix,
falling back to HOME/.local/share/vix. The stable PATH link is
HOME/.local/bin/vix and points through current; the installer does not edit
shell startup files. The installer executable also embeds those public trust
and source inputs at release build time, so replacing sidecar files cannot
replace the trust anchor. Its internal --validate-provisioning check performs
local validation only and does not fetch network data.

Users may also download and extract the current-Vix archive manually. Such an
installation is archive/manual owned and is not replaced by vix upgrade.
External package managers remain their owner's update path.

Shell completion ships under share/completions/{bash,zsh,fish} inside the
distribution. Users can source/install those assets with their shell's normal
user-level mechanism; the installer does not modify shell configuration.

The distribution assembly script stages only an explicitly built `vix` CLI,
an installed canonical Vix SDK prefix, the release public trust store, the
stable-channel source configuration, the non-glibc runtime libraries resolved
for the CLI and bootstrap installer, and the current-Vix shell completion
assets. It includes Vix and CLI license files plus copyright notices for
bundled system runtime libraries under share/licenses/runtime. It does not
sweep repository modules into a release.

The release authority owns the Ed25519 private key outside this repository.
Release automation supplies its decoded key bytes to the internal
`sign_release_manifest` boundary; neither the assembler nor a developer
installation stores that key. The installed `releases.toml` contains public
keys only. The signed manifest binds the exact artifact digest and location;
hosting is not a trust root.

Before official bootstrap is enabled, release operations must provision the
production public key and stable source URLs, build and inspect the explicit
distribution tree, produce the tar.gz artifact, generate/sign the format-1
manifest, and complete the managed A-to-B upgrade/rollback smoke. Until then,
the checked-in test trust key and `.invalid` URLs are test-only and must never
be used as official release inputs.

The initial workflow expects the HTTPS releases root for the selected host
through VIX_OFFICIAL_RELEASE_BASE_URL and appends the stable latest-download
paths for the manifest and signature. Missing source or public trust
provisioning fails before assembly. The installer embeds these public inputs;
the release private key is available only to the signing step.

The first verified release matrix is intentionally only x86_64-linux-gnu.
Release qualification checks the tag against the canonical SDK and CLI
versions, verifies root-pinned submodule revisions, assembles an explicit
runtime-library set, and tests the extracted archive and SDK consumer outside
the source/build tree. It records ldd/readelf evidence. Only x86_64-linux-gnu
is currently assembled; this does not qualify another declared host tuple.
Direct archive installs use archive ownership and are not self-updated.

The qualification workflow uses GitHub build-provenance attestations for the
host archive, bootstrap archive, release tool and build identity metadata. See
`release-artifact-verification.md` for consumer-side verification commands.
Official publication remains disabled until the protected signing environment,
source provisioning, real HTTPS bootstrap/upgrade/rollback smoke and required
platform evidence have been reviewed and validated. Existing distribution
runtime tests exercise signatures, digests, extraction and rollback through
test transports; they are not a real HTTPS end-to-end test.

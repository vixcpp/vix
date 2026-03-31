# Crypto

The `vix::crypto` module provides modern, explicit cryptographic primitives for:

- Hashing (SHA-256)
- Authenticated encryption (AEAD)
- Digital signatures (Ed25519)
- Secure randomness

This is a guide: it explains how to use each primitive with minimal code patterns.

## Include

```cpp
#include <vix/crypto/crypto.hpp>
```

Namespace:

```cpp
using namespace vix::crypto;
```

---

# Result model

Crypto operations return a result object.

- Check success with `.ok()`
- Read output with `.value()`
- Read error with `.error().message`

Minimal pattern:

```cpp
auto r = sha256(msg, out);
if (!r.ok())
{
  std::cerr << r.error().message << "\n";
  return 1;
}
```

---

# 1. Hashing

## SHA-256

Use SHA-256 when you need a deterministic digest of a message.

```cpp
#include <vix/crypto/crypto.hpp>
#include <array>
#include <cstdint>
#include <string_view>

using namespace vix::crypto;

std::array<std::uint8_t, 32> out{};
auto r = sha256(std::string_view("hello"), out);
if (!r.ok())
{
  // handle error
}

auto hex = hex_lower(out);
```

Notes:

- Output must be 32 bytes.
- `hex_lower()` formats bytes to lowercase hex.

---

# 2. Randomness

Use `random_bytes(buf)` to fill a buffer with cryptographically secure random bytes.
This is required for keys, nonces, salts, and tokens.

```cpp
std::array<std::uint8_t, 32> key{};
auto r = random_bytes(key);
if (!r.ok())
{
  // handle error
}
```

Always check `.ok()`.

---

# 3. Authenticated encryption (AEAD)

AEAD provides:

- Encryption (confidentiality)
- Integrity and authentication (tamper detection)

## Algorithms

Example algorithm:

```cpp
AeadAlg::aes_256_gcm
```

## Key and nonce sizes

For AES-256-GCM:

- key: 32 bytes
- nonce: 12 bytes
- nonce must be unique per message for a given key

## Seal (encrypt)

`seal()` encrypts and authenticates the message.
AAD is optional authenticated metadata.

```cpp
constexpr AeadAlg alg = AeadAlg::aes_256_gcm;

std::array<std::uint8_t, 32> key{};
std::array<std::uint8_t, 12> nonce{};

random_bytes(key);
random_bytes(nonce);

std::string_view msg = "hello";
std::string_view aad = "context";

auto sealed = aead::seal(alg, key, nonce, msg, aad);
if (!sealed.ok())
{
  // handle error
}
```

## Open (decrypt)

`open_string()` verifies authenticity first.
If verification fails, you get an error.

```cpp
auto open = aead::open_string(alg, key, nonce, sealed.value(), aad);
if (!open.ok())
{
  // wrong key, wrong nonce, wrong aad, or tampered ciphertext
}

std::string plaintext = open.value();
```

Rules:

- Never reuse a nonce with the same key.
- Always pass the same AAD on decrypt if you used AAD on encrypt.

---

# 4. Digital signatures

Use signatures for:

- Authenticity
- Integrity
- Identity and public-key systems

## Algorithms

Example algorithm:

```cpp
SignatureAlg::ed25519
```

## Key generation

```cpp
auto kp = signature::keygen(SignatureAlg::ed25519);
if (!kp.ok())
{
  // handle error
}

// kp.value().public_key
// kp.value().private_key
```

## Sign

```cpp
std::string_view msg = "hello";

auto sig = signature::sign(
  SignatureAlg::ed25519,
  kp.value().private_key,
  msg
);

if (!sig.ok())
{
  // handle error
}
```

## Verify

```cpp
auto ok = signature::verify(
  SignatureAlg::ed25519,
  kp.value().public_key,
  msg,
  sig.value()
);

if (!ok.ok())
{
  // invalid signature or wrong public key
}
```

---

# Recommended usage

- SHA-256: deterministic hashing (ids, caches, content fingerprints)
- AEAD (AES-256-GCM): encrypt + authenticate data and messages
- Ed25519: signatures for identity and message authenticity
- random_bytes: keys, nonces, salts, tokens

Key rules:

- Always check `.ok()`
- Never reuse AEAD nonces with the same key
- Treat private keys as secrets and zeroize if your environment requires it



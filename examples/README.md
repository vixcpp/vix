# Vix 3 examples

These examples demonstrate the current Vix C++ platform APIs. Historical
Vix2 application/framework examples were removed; the Vix 3 examples are
organized by stable Domain.

Build against an installed Vix SDK:

```sh
cmake -S examples -B build-vix-examples -DCMAKE_PREFIX_PATH=/path/to/vix
cmake --build build-vix-examples --parallel 2
```

The installed SDK determines which targets are available. Standard,
Concurrency, Files, and Process examples require their corresponding Vix SDK
targets. Network examples are added only when the SDK exports
`vix::vix_network`.

The examples are independent programs, not an application framework. Some
Network and process examples perform real I/O or invoke `git`; consult each
source file's header comment before running it.

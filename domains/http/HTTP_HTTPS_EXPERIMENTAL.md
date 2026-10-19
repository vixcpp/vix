# Experimental HTTP over TLS

The optional `HTTPSTLSExperimental` package component adds `vix::http::https_exchange`,
`vix::http::https_accept`, and `HttpsServerSession`. It is an experimental Linux
integration between the existing HTTP/1.1 and TLS Domains, not a framework.

Client calls require a `TlsClientConfig`; that configuration requires a reference
identity and uses system trust by default. Server calls require a `TlsServerConfig`.
Handshake/authentication must complete before HTTP application data is sent or a
server session is returned. Authentication failures are terminal; there is no
plaintext fallback.

The client and server operations reuse Network's provider progress and
`network::run`. A successful server response includes the TLS `close_notify`
exchange before its operation completes. TLS truncation is an error, not HTTP
message completion. The existing HTTP one-shot framing, informational-response,
body, header, and 30-second exchange bounds apply. HTTPS close-delimited HTTP
responses, streaming bodies, persistence, pooling, upgrades, and CONNECT tunnels
remain unsupported.

Build and install this component only with all three explicit options:

```sh
-DVIX_BUILD_HTTP_EXPERIMENTAL=ON \
-DVIX_BUILD_TLS_EXPERIMENTAL=ON \
-DVIX_BUILD_HTTP_TLS_EXPERIMENTAL=ON
```

For installation, also enable installation and set
`VIX_INSTALL_HTTP_EXPERIMENTAL=ON`, `VIX_INSTALL_TLS_EXPERIMENTAL=ON`, and
`VIX_INSTALL_HTTP_TLS_EXPERIMENTAL=ON`. Consumers select
`find_package(Vix REQUIRED COMPONENTS HTTPSTLSExperimental)`. Only this
component selection discovers OpenSSL; HTTP-only consumers remain independent
of TLS and OpenSSL. Linux is the only advertised platform. These names and
semantics remain provisional and are not stable-release commitments.

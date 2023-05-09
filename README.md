# Optimized TCP, TLS, QUIC & HTTP3 transports

µSockets is the non-blocking, thread-per-CPU foundation library used by [µWebSockets](https://github.com/uNetworking/uWebSockets). It provides optimized networking - using the same opaque API (programming interface) across all supported transports, event-loops and platforms (QUIC is work-in-progress, so is io_uring).

<a href="https://github.com/uNetworking/uSockets/releases"><img src="https://img.shields.io/github/v/release/uNetworking/uSockets"></a>

## Write code once
Based on µSockets, apps like µWebSockets can run on many platforms, over many transports and with many event-loops - all without any code changes or special execution paths. Moving data over TCP is just as easy as over QUIC.

Hit `make examples` to get started.

## Lightweight or featureful
In its minimal, TCP-only, configuration µSockets has no dependencies other than the very OS kernel and compiles down to a tiny binary. In its full configuration it depends on BoringSSL, lsquic and potentially some event-loop library.

Here are some configurations; WITH_IO_URING, WITH_LIBUV, WITH_ASIO, WITH_GCD, WITH_ASAN, WITH_QUIC, WITH_BORINGSSL, WITH_OPENSSL, WITH_WOLFSSL.

## Fast & stable
µWebSockets itself is known to have run with outstanding performance and stability since 2016. This thanks to, among other factors, the speed and stability of µSockets. We fuzz and randomly "hammer test" the library as part of security & stability testing done in the µWebSockets project.

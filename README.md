# Optimized TCP, TLS, QUIC & HTTP3 transports

µSockets is the non-blocking single-threaded foundation library used by [µWebSockets](https://github.com/uNetworking/uWebSockets). It provides optimized networking - using the same opaque API (programming interface) across all supported transports, event-loops and platforms (QUIC is work-in-progress).

<a href="https://github.com/uNetworking/uSockets/releases"><img src="https://img.shields.io/github/v/release/uNetworking/uSockets"></a> <a href="https://lgtm.com/projects/g/uNetworking/uSockets/context:cpp"><img alt="Language grade: C/C++" src="https://img.shields.io/lgtm/grade/cpp/g/uNetworking/uSockets.svg?logo=lgtm&logoWidth=18"/></a>

## ⏳ Write code once
Based on µSockets, apps like µWebSockets can run on many platforms, over many transports and with many event-loops - all without any code changes or special execution paths. Moving data over TCP is just as easy as over QUIC.

Hit `make examples` to get started.

## 🪶 Lightweight or featureful
In its minimal, TCP-only, configuration µSockets has no dependencies other than the very OS kernel and compiles down to a tiny binary. In its full configuration it depends on BoringSSL, lsquic and potentially some event-loop library.

Here are some configurations; WITH_LIBUV, WITH_ASIO, WITH_GCD, WITH_ASAN, WITH_QUIC, WITH_BORINGSSL, WITH_OPENSSL, WITH_WOLFSSL.

## :zap: Fast & stable
µWebSockets itself is known to run with outstanding performance and stability since 2016. This of course directly depends on the speed and stability of µSockets. We fuzz and randomly "hammer test" the library as part of security & stability testing done in µWebSockets.

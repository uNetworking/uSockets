## µSockets - miniscule networking & eventing

This is the cross-platform async networking and eventing foundation library used by [µWebSockets](https://github.com/uNetworking/uWebSockets).

### Key aspects

* Built-in (optionally available) TLS support exposed with identical interface as for TCP.
* Acknowledges and integrates with any event-loop via a layered hierarchical design of plugins.
* Extremely pedantic about user space memory footprint and designed to perform as good as can be.
* Designed from scratch to map well to user space TCP stacks or other experimental platforms.
* Low resolution timer system ideal for performant tracking of networking timeouts.
* Minimal yet truly cross-platform, will not emit a billion different platform specific error codes.
* Fully opaque library, inclusion will not completely pollute your global namespace.

### Extensible

Designed in layers of abstraction where any one layer depends only on the previous one. Write plugins and swap things out with compiler flags as you see fit.

![](misc/layout.png)

##### Available plugins
* Compile with LIBUS_USE_EPOLL to run natively on Linux (the default).
* Compile with LIBUS_USE_LIBUV to run on the libuv event-loop.
* Compile with LIBUS_USE_OPENSSL to use OpenSSL 1.1+ for crypto (the default).
* Compile with LIBUS_NO_SSL to leave the crypto layer unimplemented.

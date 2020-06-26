## µSockets - miniscule networking & eventing

This is the cross-platform async networking and eventing foundation library used by [µWebSockets](https://github.com/uNetworking/uWebSockets).

<a href="https://github.com/uNetworking/uSockets/releases"><img src="https://img.shields.io/github/v/release/uNetworking/uSockets"></a> <a href="https://lgtm.com/projects/g/uNetworking/uSockets/context:cpp"><img alt="Language grade: C/C++" src="https://img.shields.io/lgtm/grade/cpp/g/uNetworking/uSockets.svg?logo=lgtm&logoWidth=18"/></a>

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

### Compilation
Build example binaries using `make examples`. The static library itself builds with `make`. It is also possible to simply include the `src` folder in your project as it is standard C11. Defining LIBUS_NO_SSL (-DLIBUS_NO_SSL) will disable OpenSSL 1.1+ support/dependency (not needed if building with shipped Makefile). Build with environment variables set as shown below to configure for specific needs.

##### Available plugins
* Build using `WITH_LIBUV=1 make [examples]` to use libuv as event-loop.
* Build using `WITH_GCD=1 make [examples]` to use Grand Central Dispatch/CoreFoundation as event-loop (slower).
* Build using `WITH_OPENSSL=1 make [examples]` to enable and link OpenSSL 1.1+ support (or BoringSSL).
* Build using `WITH_WOLFSSL=1 make [examples]` to enable and link WolfSSL 2.4.0+ support for embedded use.

The default event-loop is native epoll on Linux, native kqueue on macOS and finally libuv on Windows.

##### A word on performance
This library is opaque; there are function calls for everything - even simple things like accessing the "user data" of a socket. In other words, static linking and link-time-optimizations mean **everything** for performance. I've benchmarked dynamic linking vs. static, link-time optimized ones and found close to 100% performance difference.

Also, the kernel you run makes a huge difference. Mitigations off, or a modern hardware-mitigated CPU makes all the difference and distros like Clear Linux have shown huge speedups compared to more "vanilla" kernels.

Whatever you do, **never** ever link to uSockets dynamically.

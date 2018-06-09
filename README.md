## µSockets - miniscule networking & eventing

This is a cross-platform async networking and eventing foundation library written in standard C. It is the heart and soul of [µWebSockets](https://github.com/uNetworking/uWebSockets) and outperforms (esp. in memory footprint) just about everything similar out there (libuv, ASIO, etc.).

Read the [docs](misc/manual.md) for an overview.

* Built-in SSL support with similar interface as for non-SSL makes it a breeze building secure servers and clients.
* Integrates with any event-loop via a layered hierarchical design of plugins.
* Maps well to other user space TCP stacks (wip).
* Extremely pedantic about user space memory footprint.

Non-SSL | SSL
--- | ---
![](misc/http.png) | ![](misc/https.png)

A sample HTTP & HTTPS server has been built to very thoroughly benchmark, track and compare many cases of IO against the well-known Node.js v10. It reliably wins in every possible test case, most significantly at smaller response sizes where user space plays a larger role than when just blasting the Linux kernel with large static data buffers.

One can easily conclude that for servers sending large static data, Node.js will do just as good as any other C server. For everything else there is definitely room for major improvements not only in raw throughput but esp. in memory footprint.

### Extensible

Designed in layers of abstraction where any one layer depends only on the previous one. Write plugins and swap things out with compiler flags as you see fit.

![](misc/layout.png)

##### Available plugins
* Compile with LIBUS_USE_EPOLL to run natively on Linux (the default).
* Compile with LIBUS_USE_LIBUV to run on the libuv event-loop.
* Compile with LIBUS_USE_OPENSSL to use OpenSSL 1.1+ for crypto (the default).
* Compile with LIBUS_NO_SSL to leave the crypto layer unimplemented.

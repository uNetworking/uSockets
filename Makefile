DESTDIR ?=

prefix ?= "/usr/local"
exec_prefix	?=	"$(prefix)"
libdir ?=	"$(exec_prefix)/lib"
includedir?=	"$(exec_prefix)/include/uSockets"

VERSION = 0.3.5
LIBTARGET = libusockets.so


# WITH_OPENSSL=1 enables OpenSSL 1.1+ support
ifeq ($(WITH_OPENSSL),1)
	override CFLAGS += -DLIBUS_USE_OPENSSL
	# With problems on macOS, make sure to pass needed LDFLAGS required to find these
	override LDFLAGS += -lssl -lcrypto
else
	# WITH_WOLFSSL=1 enables WolfSSL 4.2.0 support (mutually exclusive with OpenSSL)
	ifeq ($(WITH_WOLFSSL),1)
		# todo: change these
		override CFLAGS += -DLIBUS_USE_WOLFSSL -I/usr/local/include
		override LDFLAGS += -L/usr/local/lib -lwolfssl
	else
		override CFLAGS += -DLIBUS_NO_SSL
	endif
endif

# WITH_LIBUV=1 builds with libuv as event-loop
ifeq ($(WITH_LIBUV),1)
	override CFLAGS += -DLIBUS_USE_LIBUV
	override LDFLAGS += -luv
endif

# WITH_GCD=1 builds with libdispatch as event-loop
ifeq ($(WITH_GCD),1)
	override CFLAGS += -DLIBUS_USE_GCD
	override LDFLAGS += -framework CoreFoundation
endif

# WITH_ASAN builds with sanitizers
ifeq ($(WITH_ASAN),1)
	override CFLAGS += -fsanitize=address
	override LDFLAGS += -lasan
endif

override CFLAGS += -std=c11 -Isrc
override LDFLAGS += uSockets.a

# By default we build the libusockets.so shared library
default:
	rm -f *.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -fpic -c src/*.c src/eventing/*.c src/crypto/*.c
	$(CC) -shared -Wl,-soname,libusockets.so -o $(LIBTARGET) *.o

install: default
	# install the folders needed  (making sure that the exist)
	install -d "$(DESTDIR)$(libdir)" \
	"$(DESTDIR)$(includedir)/internal/eventing" \
	"$(DESTDIR)$(includedir)/internal/networking"
	# install the library first, while making sure that the symlink is updated
	install -m 755 $(LIBTARGET) "$(DESTDIR)$(libdir)/$(LIBTARGET).$(VERSION)"
	@ cd "$(DESTDIR)$(libdir)" && ln -snf "$(LIBTARGET).$(VERSION)" "$(LIBTARGET)"
	# we also install all the header files
	install -m 644 src/*.h "$(DESTDIR)$(includedir)/"
	install -m 644 src/internal/*.h "$(DESTDIR)$(includedir)/internal/"
	install -m 644 src/internal/eventing/*.h "$(DESTDIR)$(includedir)/internal/eventing/"
	install -m 644 src/internal/networking/*.h "$(DESTDIR)$(includedir)/internal/networking/"

static:
	rm -f *.o
	$(CC) $(CFLAGS) -flto -O3 -c src/*.c src/eventing/*.c src/crypto/*.c
	$(AR) rvs uSockets.a *.o

# Builds all examples
.PHONY: examples
examples: default
	for f in examples/*.c; do $(CC) -flto -O3 $(CFLAGS) -o $$(basename "$$f" ".c") "$$f" $(LDFLAGS); done

swift_examples:
	swiftc -O -I . examples/swift_http_server/main.swift uSockets.a -o swift_http_server

clean:
	rm -f *.o
	rm -f *.a
	rm -f *.so
	rm -rf .certs

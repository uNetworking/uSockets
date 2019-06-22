# WITH_SSL=1 enables SSL support
ifneq ($(WITH_SSL),1)
	override CFLAGS += -DLIBUS_NO_SSL
else
	# With problems on macOS, make sure to pass needed LDFLAGS required to find these
	override LDFLAGS += -lssl -lcrypto
endif

# WITH_LIBUV=1 builds with libuv as event-loop
ifeq ($(WITH_LIBUV),1)
	override CFLAGS += -DLIBUS_USE_LIBUV
	override LDFLAGS += -luv
else
	# macOS requires it anyways
	ifeq ($(shell uname -s), Darwin)
		override LDFLAGS += -luv
	endif
endif

override CFLAGS += -std=c11 -Isrc
override LDFLAGS += uSockets.a

# By default we build the uSockets.a static library
default:
	rm -f *.o
	$(CC) $(CFLAGS) -flto -O3 -c src/*.c src/eventing/*.c
	$(AR) rvs uSockets.a *.o

# Builds all examples
.PHONY: examples
examples: default
	for f in examples/*.c; do $(CC) -flto -O3 $(CFLAGS) -o $$(basename "$$f" ".c") "$$f" $(LDFLAGS); done

clean:
	rm -f *.o
	rm -f *.a

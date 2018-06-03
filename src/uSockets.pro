TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.c \
    libusockets.c \
    backends/epoll.c \
    context.c \
    socket.c \
    backends/libuv.c

HEADERS += \
    libusockets.h \
    internal/epoll.h \
    internal/bsd.h \
    internal/libuv.h \
    internal/common.h

#QMAKE_CFLAGS += -fsanitize=address# -DLIBUS_USE_LIBUV -Wno-unused-parameter
#LIBS += -lasan -luv
#LIBS += -lasan -lssl -lcrypto
LIBS += -lssl -lcrypto

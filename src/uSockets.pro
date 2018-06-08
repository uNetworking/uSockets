TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.c \
    libusockets.c \
    eventing/epoll.c \
    context.c \
    socket.c \
    eventing/libuv.c \
    ssl.c

HEADERS += \
    libusockets.h \
    internal/eventing/epoll.h \
    internal/networking/bsd.h \
    internal/eventing/libuv.h \
    internal/common.h \
    interfaces/timer.h \
    interfaces/socket.h \
    interfaces/poll.h \
    interfaces/context.h \
    interfaces/loop.h \
    interfaces/ssl.h

QMAKE_CFLAGS_DEBUG += -Wno-unused-parameter
QMAKE_CFLAGS += -Wno-unused-parameter
QMAKE_CXXFLAGS += -Wno-unused-parameter
#QMAKE_CFLAGS += -fsanitize=address# -DLIBUS_USE_LIBUV
#LIBS += -lasan -luv
#LIBS += -lasan -lssl -lcrypto
LIBS += -lssl -lcrypto

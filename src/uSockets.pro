TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.c \
    eventing/epoll.c \
    context.c \
    socket.c \
    eventing/libuv.c \
    ssl.c \
    loop.c

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
    interfaces/ssl.h \
    internal/loop.h

#QMAKE_CFLAGS_DEBUG += -Wno-unused-parameter
#QMAKE_CFLAGS += -Wno-unused-parameter
#QMAKE_CXXFLAGS += -Wno-unused-parameter
QMAKE_CFLAGS += -fsanitize=address -DLIBUS_NO_SSL -DLIBUS_USE_LIBUV
QMAKE_LFLAGS+="-fsanitize=address"
INCLUDEPATH += "/usr/local/include"
#LIBS += -lasan -luv
#LIBS += -lasan -luv -lssl -lcrypto #-luv
#LIBS += -lssl -lcrypto
LIBS += -L/usr/local/lib -luv #-L/usr/local/opt/openssl/lib -lssl -lcrypto

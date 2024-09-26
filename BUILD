cc_library(
    name = "uSockets",
    srcs = glob([
        "src/*.c",
        "src/eventing/*.c",
        "src/crypto/*.c",
    ]),
    hdrs = glob(
        ["src/**/*.h"],
        ["src/internal/eventing/asio.h"],
    ),
    defines = ["LIBUS_NO_SSL"],
    includes = ["src"],
    deps = ["@liburing"],
)

cc_library(
    name = "usockets",
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
    visibility = ["//visibility:public"],
    deps = ["@liburing"],
)

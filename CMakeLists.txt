cmake_minimum_required(VERSION 3.16)

# If project is not already defined, define it (for monorepo)
if(NOT CMAKE_PROJECT_NAME)
    project(uSockets C CXX)
endif()
message(STATUS "Building uSockets")

# Set C standard to C11 and C++ standard to C++17
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Default build type is Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compile options based on environment variables
if(NOT DEFINED WITH_LTO)
    set(WITH_LTO 1)
endif()

if(WITH_LTO)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
endif()

if(WITH_ASAN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -g")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif()

# Set default flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -march=native")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")

# Add source directories
set(SRC_DIR src)

# Add library sources
file(GLOB_RECURSE LIB_SOURCES "${SRC_DIR}/*.c")
file(GLOB_RECURSE LIB_HEADERS "${SRC_DIR}/*.h")

# Add include headers
include_directories(${SRC_DIR})

# Create the static library
add_library(uSockets STATIC ${LIB_SOURCES} ${LIB_HEADERS})

# Optionally build with BoringSSL, OpenSSL, WolfSSL, or other features
if(WITH_BORINGSSL)
    add_compile_definitions(LIBUS_USE_OPENSSL)
    target_include_directories(uSockets PRIVATE boringssl/include)
    target_link_libraries(uSockets boringssl/build/ssl/libssl.a boringssl/build/crypto/libcrypto.a pthread)
elseif(WITH_OPENSSL)
    add_compile_definitions(LIBUS_USE_OPENSSL)
    target_link_libraries(uSockets ssl crypto pthread)
elseif(WITH_WOLFSSL)
    add_compile_definitions(LIBUS_USE_WOLFSSL)
    target_include_directories(uSockets PRIVATE /usr/local/include)
    target_link_libraries(uSockets wolfssl)
else()
    add_compile_definitions(LIBUS_NO_SSL)
endif()

# Add event loop options
if(WITH_LIBUV)
    find_package(libuv CONFIG REQUIRED)
    add_compile_definitions(LIBUS_USE_LIBUV)
    target_link_libraries(uSockets PUBLIC $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>)
endif()

if(WITH_ASIO)
    add_compile_definitions(LIBUS_USE_ASIO)
    target_link_libraries(uSockets pthread stdc++)
endif()

if(WITH_IO_URING)
    add_compile_definitions(LIBUS_USE_IO_URING)
    target_link_libraries(uSockets /usr/lib/liburing.a)
endif()

if(WITH_GCD)
    add_compile_definitions(LIBUS_USE_GCD)
    target_link_libraries(uSockets CoreFoundation)
endif()

# Add sources for optional QUIC support
if(WITH_QUIC)
    add_compile_definitions(LIBUS_USE_QUIC)
    target_include_directories(uSockets PRIVATE lsquic/include)
    target_link_libraries(uSockets pthread z m lsquic/src/liblsquic/liblsquic.a)
endif()

# Build examples
file(GLOB EXAMPLES "examples/*.c")
foreach(EXAMPLE_FILE ${EXAMPLES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_FILE} NAME_WE)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE_FILE})
    target_link_libraries(${EXAMPLE_NAME} uSockets ${CMAKE_EXE_LINKER_FLAGS})
endforeach()

# Swift example (optional)
add_custom_target(swift_examples
        COMMAND swiftc -O -I . examples/swift_http_server/main.swift uSockets.a -o swift_http_server
)

# BoringSSL build target
add_custom_target(boringssl
        COMMAND ${CMAKE_COMMAND} -E make_directory boringssl/build
        COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release -Bboringssl/build -Hboringssl
        COMMAND cmake --build boringssl/build --target all
)

# Clean target
add_custom_target(clean-all
        COMMAND ${CMAKE_COMMAND} -E remove -f *.o *.a swift_http_server
        COMMAND ${CMAKE_COMMAND} -E remove_directory .certs
)

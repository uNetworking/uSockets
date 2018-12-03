cmake_minimum_required(VERSION 3.8)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(proj_name usockets)
project(${proj_name} C)


if (CMAKE_BUILD_TYPE EQUAL "DEBUG")
    message("debug mode")
    set(CMAKE_CXX_FLAGS "-g ${CMAKE_CXX_FLAGS}") 
endif (CMAKE_BUILD_TYPE EQUAL "DEBUG")

add_library(${proj_name} STATIC
	src/context.c
	src/loop.c
	src/socket.c
	src/ssl.c
	src/eventing/epoll.c
	src/eventing/libuv.c
)
target_include_directories(${proj_name} PUBLIC src)
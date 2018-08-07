default:
	gcc -DLIBUS_NO_SSL -O3 -o echo_server -s -Isrc src/*.c src/eventing/*.c examples/echo_server.c
	gcc -DLIBUS_NO_SSL -O3 -o http_server -s -Isrc src/*.c src/eventing/*.c examples/http_server.c
	gcc -DLIBUS_NO_SSL -O3 -o hammer_test -s -Isrc src/*.c src/eventing/*.c examples/hammer_test.c
	gcc -DLIBUS_NO_SSL -DLIBUS_USE_LIBUV -O3 -o hammer_test_libuv -s -Isrc src/*.c src/eventing/*.c examples/hammer_test.c -luv

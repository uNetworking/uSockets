default:
	gcc -DLIBUS_NO_SSL -O3 -o echo_server -s -Isrc src/*.c src/eventing/*.c examples/echo_server.c
	gcc -DLIBUS_NO_SSL -O3 -o http_server -s -Isrc src/*.c src/eventing/*.c examples/http_server.c

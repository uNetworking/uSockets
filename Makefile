default:
	gcc -O3 -DLIBUS_NO_SSL -o echo_server -s -Isrc src/*.c src/eventing/*.c examples/echo_server.c -lssl -lcrypto

default:
	gcc -O3 -o echo_server -s -Isrc src/*.c src/eventing/*.c examples/echo_server.c -lssl -lcrypto

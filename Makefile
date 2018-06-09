default:
	gcc -O3 -o test_server -s -Isrc src/*.c src/eventing/*.c -lssl -lcrypto

default:
	for f in examples/*.c; do gcc -flto -std=c11 -DLIBUS_NO_SSL -O3 -o $$(basename "$$f" ".c") -Isrc src/*.c src/eventing/*.c "$$f"; done

libuv:
	for f in examples/*.c; do gcc -flto -std=c11 -DLIBUS_NO_SSL -DLIBUS_USE_LIBUV -O3 -o $$(basename "$$f" ".c") -Isrc src/*.c src/eventing/*.c -luv "$$f"; done

ssl:
	for f in examples/*.c; do gcc -flto -std=c11 -O3 -o $$(basename "$$f" ".c") -Isrc src/*.c src/eventing/*.c -lssl -lcrypto "$$f"; done

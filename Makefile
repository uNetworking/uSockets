default:
	for f in examples/*.c; do gcc -std=c11 -DLIBUS_NO_SSL -O3 -o $$(basename "$$f" ".c") -s -Isrc src/*.c src/eventing/*.c "$$f"; done

ssl:
	for f in examples/*.c; do gcc -std=c11 -O3 -o $$(basename "$$f" ".c") -g -Isrc src/*.c src/eventing/*.c -lssl -lcrypto "$$f"; done

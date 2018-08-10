default:
	for f in examples/*.c; do gcc -std=c11 -DLIBUS_NO_SSL -O3 -o $$(basename "$$f" ".c") -s -Isrc src/*.c src/eventing/*.c "$$f"; done


build:
	gcc crank.c -O3 -o crank -lsecp256k1 -pthread -D_GNU_SOURCE

run: build
	./crank
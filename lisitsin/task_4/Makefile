all: dynamic static

dynamic: dynamic.c
	mpicc -lm -std=c99 dynamic.c -o dynamic

static: static.c
	mpicc -lm -std=c99 static.c -o static

clean:
	rm -f dynamic static

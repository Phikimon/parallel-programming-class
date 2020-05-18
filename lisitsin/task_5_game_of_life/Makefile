all: main

main: main.c
	mpicc -g -lm -std=c99 -lSDL2 -lSDL2main main.c -o main 2> /dev/null \
	|| mpicc -g -lm -std=c99 main.c -o main

clean:
	rm -f main

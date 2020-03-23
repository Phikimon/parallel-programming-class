IMPLS=dummy filipp_tas

IMPLTARGETS=$(addprefix build/, $(IMPLS))

all: $(IMPLTARGETS)

IMPLDIR=implementations
BNCHDIR=benchmark
SHELL=/bin/bash

$(IMPLTARGETS): build/% : implementations/%/lock.o build build/main.o
	gcc -pthread build/main.o $(IMPLDIR)/$(@:build/%=%)/lock.o -o $@

build/main.o: $(BNCHDIR)/main.c
	gcc -c -pthread -std=c99 $^ -o $@

%lock.o: %lock.c
	gcc -Wall -pedantic -c -static -Ibenchmark/ -shared -std=c99 $^ -o $@

build:
	mkdir build/

clean:
	rm -rf build/ implementations/*/lock.o

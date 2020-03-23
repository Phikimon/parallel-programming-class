IMPLS=dummy filipp_tas

IMPLTARGETS=$(addprefix build/, $(IMPLS))

all: $(IMPLTARGETS)

IMPLDIR=implementations
BNCHDIR=benchmark
SHELL=/bin/bash

$(IMPLTARGETS): build/% : implementations/%/object build build/main.o
	gcc -pthread build/main.o $(IMPLDIR)/$(@:build/%=%)/*.o -o $@

build/main.o: $(BNCHDIR)/main.c
	gcc -c -pthread -std=c99 $^ -o $@

%object: %Makefile
	make -s -C $* all

build:
	mkdir build/

clean:
	rm -rf build/ implementations/*/*.o

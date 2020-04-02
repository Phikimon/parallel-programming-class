IMPLS=filipp_tas              \
      filipp_ttas             \
      filipp_ttas_pause       \
      filipp_ttas_wait        \
      filipp_queue_shared     \
      filipp_queue_excl       \
      filipp_queue_no_deref   \
      filipp_queue_pause_atom \
      filipp_queue_pause      \
      filipp_queue_wait

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

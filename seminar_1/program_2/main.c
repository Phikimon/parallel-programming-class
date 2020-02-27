// for dprintf()
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <stdint.h>

// This file uses gcc-specific features, such as
// built-in atomics and memory fences. C11 could
// be used instead but nobody loves it and neither do I.

#define mfence()                  __sync_synchronize()
#define atomic_store(ptr, val)    __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_load(ptr)          __atomic_load_n(ptr, __ATOMIC_SEQ_CST)
#define atomic_fetch_and_inc(ptr) ((int32_t)(__atomic_fetch_add(ptr, 1, __ATOMIC_SEQ_CST)))

// NTHREADS should not be more than 9,
// otherwise characters being printed
// won't be digits
#define NTHREADS 9
#define NCHARS   1000

char buf[NCHARS * NTHREADS] = {0};

void* writer(void* arg);
void* reader(void* arg);

/* +---+---+---+---+---+---+---+ I - inconsistent
 * | C | C | I | I | E | E | E | C - consistent
 * +---+---+---+---+---+---+---+ E - empty
 *           ^       ^
 *         head     tail
 *
 * head and tail are atomic counters shared
 * by writers and readers. Writers "acquire"
 * byte in buf[] by fetch_and_inc() operation,
 * and the value of tail that this instruction
 * returned is the index of acquired byte.
 * Once byte is consistently written(this is
 * assured by memory fence) head is advanced
 * as well, informing readers about new data
 * in the buffer.
 * These atomics are aligned to typical cache
 * line size to avoid false sharing. */
volatile int32_t tail __attribute__ ((aligned(64))) = 0;
volatile int32_t head = 0;

int main(void)
{
	int r;

	atomic_store(&tail, 0);
	atomic_store(&head, 0);
	pthread_t thr[2 * NTHREADS];
	// Spawn writers
	for (int i = 0; i < NTHREADS; i++) {
		r = pthread_create(&thr[i], NULL, writer, (void*)(i + 1));
		assert((r == 0) && "pthread_create(writer)");
	}
	// Spawn readers
	for (int i = 0; i < NTHREADS; i++) {
		r = pthread_create(&thr[NTHREADS + i], NULL, reader, (void*)(i + 1));
		assert((r == 0) && "pthread_create(reader)");
	}
	for (int i = 0; i < 2 * NTHREADS; i++) {
		r = pthread_join(thr[i], NULL);
		assert((r == 0) && "pthread_join");
	}

	return 0;
}

void* writer(void* arg)
{
	char mychar = '0' + (int)arg;
	int cnt;
	int32_t loaded_head;
	for (int i = 0; i < NCHARS; i++) {
		// 'Acquire' byte
		cnt = atomic_fetch_and_inc(&tail);
		assert((cnt >= 0) && (cnt < NCHARS * NTHREADS));
		buf[cnt] = mychar;
		/* This memory fence guarantees that
		 * readers will see the write to buf */
		mfence();
		/* Wait while all bytes with indexes less
		   than cnt become consistent. cas not necessary */
		while (1) {
			loaded_head = atomic_load(&head);
			if (loaded_head == cnt)
				break;
			assert(loaded_head < cnt);
		}
		// Inform readers about new data
		loaded_head = atomic_fetch_and_inc(&head);
		/* We are the only one who could move head from
		   current position */
		assert(loaded_head == cnt);
	}

	return NULL;
}

void* reader(void* arg)
{
	char mychar = '0' + (int)arg;
	char filename[] = "_.txt";
	filename[0] = mychar;
	int file = open(filename, O_CREAT|O_TRUNC|O_WRONLY, 0666);

	int written_to_file = 0;
	int cnt = 0;
	int32_t loaded_head = 0;
	while (written_to_file < NCHARS) {
		/* This tightloop affects performance
		   severely but let's mess around
		   while we can :)
		   As a fix we might want to introduce
		   conditional variable, but the time
		   of "creating new data" in buf is so
		   small that it will not help much. */
		while (loaded_head <= cnt)
			loaded_head = atomic_load(&head);
		if (buf[cnt++] == mychar) {
			dprintf(file, "%c", mychar);
			written_to_file++;
		}
	}

	fsync(file);
	close(file);
	return NULL;
}

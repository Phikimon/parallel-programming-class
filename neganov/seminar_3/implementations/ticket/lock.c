#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// How dare you! You stole my zero-sized arrays.
#pragma GCC diagnostic ignored "-Wzero-length-array"

#include "lock.h"

#define MAX_THREADS (unsigned)256000

typedef uint32_t ticket_t;

struct lock {
	volatile ticket_t ticket_serving;
	volatile ticket_t next_ticket;
};

#define atomic_store(ptr, val)    __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_load(ptr)          __atomic_load_n(ptr, __ATOMIC_SEQ_CST)
#define atomic_fetch_and_inc(ptr) ((uint32_t)(__atomic_fetch_add(ptr, 1, __ATOMIC_SEQ_CST)))

lock_t* lock_alloc(long unsigned n_threads) {
	if ((n_threads == 0) || (n_threads > MAX_THREADS)) {
		fprintf(stderr, "\nlock_alloc(): Too many or zero threads: 0 < n_threads = %lu < %u\n",
				n_threads, MAX_THREADS);
		return NULL;
	}
	int size = sizeof(struct lock);
	if (size < 64)
		size = 64;
	struct lock* qlock = calloc(1, size);

	return (lock_t*)qlock;
}

int lock_acquire(lock_t* arg) {
	lock_t* qlock = (lock_t*)arg;

	ticket_t my_ticket = atomic_fetch_and_inc(&qlock->next_ticket);
	while (qlock->ticket_serving != my_ticket)
		__asm volatile ("pause" :::);

	return 0;
}

int lock_release(lock_t* arg) {
	lock_t* qlock = (lock_t*)arg;

	qlock->ticket_serving++;

	return 0;
}

int lock_free(lock_t* arg) {
	lock_t* qlock = (lock_t*)arg;

	ticket_t next_ticket = atomic_load(&qlock->next_ticket);
	ticket_t ticket_serving = qlock->ticket_serving;
	if (next_ticket != ticket_serving) {
		fprintf(stderr, "\nlock_free(): next_ticket = %u != %u = ticket_serving\n",
				next_ticket, ticket_serving);
		return 1;
	}

	free(qlock);
	return 0;
}

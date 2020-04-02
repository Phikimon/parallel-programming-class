#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// How dare you! You stole my zero-sized arrays.
#pragma GCC diagnostic ignored "-Wzero-length-array"

#include "lock.h"

#define MAX_THREADS (unsigned)256000

typedef union {
		volatile uint8_t val;
		uint8_t padding[64];
} serving_t;

typedef uint32_t ticket_t;

struct lock {
	volatile ticket_t ticket_serving;
	volatile ticket_t next_ticket;
	volatile ticket_t n_threads;
	uint8_t padding[64 - sizeof(ticket_t)*3];
	// If thread with ticket N is currently
	// in the critical zone then
	// serving[N % n_threads] is set to one
	serving_t serving[0];
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
	int size = sizeof(struct lock) + n_threads * sizeof(serving_t);
	struct lock* qlock = calloc(1, size);
	if (!qlock)
		return NULL;
	qlock->serving[0].val = 1;
	qlock->n_threads = n_threads;

	return (lock_t*)qlock;
}

int lock_acquire(lock_t* arg) {
	lock_t* qlock = (lock_t*)arg;

	ticket_t my_ticket = atomic_fetch_and_inc(&qlock->next_ticket);
	volatile uint8_t* my_cell = &qlock->serving[my_ticket % qlock->n_threads].val;
	while (*my_cell != 1) {
		__asm volatile ("pause" :::);
	}

	return 0;
}

int lock_release(lock_t* arg) {
	lock_t* qlock = (lock_t*)arg;

	ticket_t my_ticket = atomic_fetch_and_inc(&qlock->ticket_serving);
	// The only thread that may have ticket (my_ticket + n_threads) is this thread,
	// so it's okay to have several values in array equal to 1 simultaneously.
	qlock->serving[my_ticket % qlock->n_threads].val = 0;
	qlock->serving[(my_ticket + 1) % qlock->n_threads].val = 1;
	return 0;
}

int lock_free(lock_t* arg) {
	lock_t* qlock = (lock_t*)arg;

	ticket_t next_ticket = atomic_load(&qlock->next_ticket);
	ticket_t ticket_serving = atomic_load(&qlock->ticket_serving);
	if (next_ticket != ticket_serving) {
		fprintf(stderr, "\nlock_free(): next_ticket = %u != %u = ticket_serving\n",
				next_ticket, ticket_serving);
		return 1;
	}

	if ((qlock->n_threads == 0) || (qlock->n_threads > MAX_THREADS)) {
		fprintf(stderr, "\nlock_free(): too many or zero threads: 0 < n_threads = %u < %u\n",
				qlock->n_threads, MAX_THREADS);
		return 1;
	}

	int32_t sum = 0;
	for (uint32_t i = 0; i < qlock->n_threads; i++) {
		sum += qlock->serving[i].val;
	}
	if (sum != 1) {
		fprintf(stderr, "\nlock_free(): sum of all serving[] values not equal to one(= %d)\n",
				sum);
		return 1;
	}

	free(qlock);
	return 0;
}

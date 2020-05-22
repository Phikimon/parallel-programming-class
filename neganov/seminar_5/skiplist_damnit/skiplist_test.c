#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "helpers.h"
#include "skiplist.h"

#define ITER_NUM 50000 // must be divisible by *_THREADS
#define THREADS_PER_OPERATION 5

#define TOTAL_THREADS (THREADS_PER_OPERATION * 3)
#define ITER_PER_CONTAINS (ITER_NUM / THREADS_PER_OPERATION)
#define ITER_PER_ADD      (ITER_NUM / THREADS_PER_OPERATION)
#define ITER_PER_REMOVE   (ITER_NUM / THREADS_PER_OPERATION)

sl_s* skiplist;

//#define DEBUG_SINGLETHREADED

#ifndef DEBUG_SINGLETHREADED
#  define DEBUG_DATA
//#  define DEBUG_DATA_VERBOSE
//#  define SHOW_PROGRESS
#endif

#ifdef DEBUG_DATA
volatile int contains_counter = 0;
volatile int add_counter = 0;
volatile int remove_counter = 0;
#endif

#define VALS_TO_STORE (10*(ITER_NUM))

void* contains(void* arg) {
	sl_s* skiplist = (sl_s*)arg;
	for (int i = 0; i < ITER_PER_CONTAINS; i++) {
#ifdef SHOW_PROGRESS
		fprintf(stderr, "%d: in contains\n", i);
#endif
#ifdef DEBUG_DATA
		int val = atomic_fetch_inc(&contains_counter);
#ifdef DEBUG_DATA_VERBOSE
		fprintf(stderr, "contains %d\n", val);
#endif
		sl_contains(skiplist, val);
#else
		int val = rand() % VALS_TO_STORE;
		sl_contains(skiplist, val);
#endif
#ifdef SHOW_PROGRESS
		fprintf(stderr, "%d: out contains\n", i);
#endif
	}
	return NULL;
}

void* add(void* arg) {
	sl_s* skiplist = (sl_s*)arg;
	for (int i = 0; i < ITER_PER_ADD; i++) {
#ifdef SHOW_PROGRESS
		fprintf(stderr, "%d: in add\n", i);
#endif
#ifdef DEBUG_DATA
		int val = atomic_fetch_inc(&add_counter);
#ifdef DEBUG_DATA_VERBOSE
		fprintf(stderr, "add %d\n", val);
#endif
		assert(sl_add(skiplist, val));
#else
		int val = rand() % VALS_TO_STORE;
		sl_add(skiplist, val);
#endif
#ifdef SHOW_PROGRESS
		fprintf(stderr, "%d: out add\n", i);
#endif
	}
	return NULL;
}

void* my_remove(void* arg) {
	sl_s* skiplist = (sl_s*)arg;
	for (int i = 0; i < ITER_PER_REMOVE; i++) {
#ifdef SHOW_PROGRESS
		fprintf(stderr, "%d: in remove\n", i);
#endif
#ifdef DEBUG_DATA
		int val = atomic_fetch_inc(&remove_counter);
#ifdef DEBUG_DATA_VERBOSE
		fprintf(stderr, "remove %d\n", val);
#endif
		while (!sl_remove(skiplist, val));
#else
		int val = rand() % VALS_TO_STORE;
		sl_remove(skiplist, val);
#endif
#ifdef SHOW_PROGRESS
		fprintf(stderr, "%d: out remove\n", i);
#endif
	}
	return NULL;
}

int main(void)
{
	skiplist = calloc(1, sizeof(sl_s));
	assert(skiplist);
	sl_init(skiplist);

#ifdef DEBUG_DATA
	atomic_store(&remove_counter, 0);
	atomic_store(&contains_counter, 0);
	atomic_store(&add_counter, 0);
#endif

#ifdef DEBUG_SINGLETHREADED
	// Stage 1
	for (int i = 0; i < ITER_NUM; i++)
		assert(sl_add(skiplist, i) == 1);
	for (int i = 0; i < ITER_NUM; i++)
		assert(sl_contains(skiplist, i) == 1);
	for (int i = 0; i < ITER_NUM; i++)
		assert(sl_remove(skiplist, i) == 1);
	assert(sl_debug_get_number_of_keys(skiplist) == 2);
	// Stage 2
	for (int i = 0; i < ITER_NUM; i++) {
		assert(sl_add(skiplist, i) == 1);
		assert(sl_contains(skiplist, i) == 1);
		assert(sl_remove(skiplist, i) == 1);
	}
	assert(sl_debug_get_number_of_keys(skiplist) == 2);
	// Stage 3
	for (int i = 0; i < ITER_NUM; i++) {
		assert(sl_remove(skiplist, i) == 0);
		assert(sl_add(skiplist, i) == 1);
		assert(sl_contains(skiplist, i) == 1);
		assert(sl_remove(skiplist, i) == 1);
		assert(sl_contains(skiplist, i) == 0);
	}
	assert(sl_debug_get_number_of_keys(skiplist) == 2);
#else
	int r;
	pthread_t thr[TOTAL_THREADS];
	// Spawn writers
	for (int i = 0; i < TOTAL_THREADS; i++) {
		void* (*f)(void*) = NULL;
		if (i < THREADS_PER_OPERATION)
			f = contains;
		else if (i < 2 * THREADS_PER_OPERATION)
			f = add;
		else f = my_remove;
		r = pthread_create(&thr[i], NULL, f, (void*)(skiplist));
		assert((r == 0) && "pthread_create");
	}
	for (int i = 0; i < TOTAL_THREADS; i++) {
		r = pthread_join(thr[i], NULL);
		assert((r == 0) && "pthread_join");
	}
#endif

	sl_debug_show_all_keys(skiplist);
	fprintf(stderr, "keys left = %d\n", sl_debug_get_number_of_keys(skiplist));
#ifdef DEBUG_DATA
	assert(sl_debug_get_number_of_keys(skiplist) == 2);
#endif
	return 0;
}
